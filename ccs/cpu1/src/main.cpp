// CPU1 composition root: system init, boot of CPU2, peripheral hand-over,
// static objects and their wiring (AGENTS.md §3.4, §5).
//
// Phase 7: cascaded voltage and current control with the generated control
// loops, one ISR each. Commands come from the debugger through controlCommand,
// results are in controlStatus. The state machine (variant B1) and the commands
// from CPU2 follow when the generated state machine is available.

#include <stdint.h>

extern "C" {
#include "device.h"
}

#include "app/Blinker.hpp"
#include "app/LinearScaling.hpp"
#include "app/Measurements.hpp"
#include "app/ControlTask.hpp"
#include "ctrl/CurrentLoop.hpp"
#include "ctrl/VoltageLoop.hpp"
#include "hal/Adc.hpp"
#include "hal/AdcInput.hpp"
#include "hal/CpuTimer.hpp"
#include "hal/EpwmTimeBase.hpp"
#include "hal/GpioOutput.hpp"
#include "hal/PowerStagePwm.hpp"
#include "hal/TripZone.hpp"
#include "ipc/SyncCores.hpp"
#include "shared_types/IpcFlags.hpp"

// Debugger interface until the state machine and CPU2 provide the commands
// (global, so CCS finds the names). Set the fields in the Expressions view.
// mode: 0 = constant voltage, 2 = constant current (ctrl::VoltageLoopInputs::Mode).
struct ControlCommand {
    bool outputEnable;
    uint16_t mode;
    float voltageSetpoint;      // V
    float currentSetpoint;      // A, constant current mode only
};

struct ControlStatus {
    bool outputEnabled;
    app::Measurements measurements;
    ctrl::ControlLoopOutputs voltageLoop;   // output = current reference (A)
    ctrl::ControlLoopOutputs currentLoop;   // output = duty cycle (0..0.95)
    uint32_t voltageIsrCount;
    uint32_t currentIsrCount;
    uint32_t voltageAdcOverflowCount;
    uint32_t currentAdcOverflowCount;
};

volatile ControlCommand controlCommand = { false, 0U, 0.0F, 0.0F };
ControlStatus controlStatus;

namespace {

const uint32_t kTickPeriodUs = 1000UL;     // 1 kHz system tick
const uint16_t kBlinkTicks = 500U;         // LED1 toggles every 500 ms

// Power stage PWM as in Project A (LabPsu_Pwm): ePWM4, TBCLK = 100 MHz,
// up-down count, 50 kHz, dead band 20 half cycles = 100 ns.
const uint16_t kPwmPeriodCounts = 1000U;   // 100 MHz / (2 * 50 kHz)
const uint16_t kDeadBandCounts = 20U;
// Control rate: SOCA at every 5th counter-period event = 10 kHz.
const uint16_t kAdcTriggerPrescale = 5U;
// ePWM5 triggers the voltage measurement 50 counts (0.5 us) after ePWM4.
const uint16_t kVoltageTriggerPhase = 50U;
// 16 SYSCLK cycles = 80 ns (>= 75 ns at 12 bit).
const uint32_t kAdcSampleWindow = 16U;

// Measurement scaling as in Project A ("scaling" subsystems): raw / 2^12 * 5.
// Placeholder until the gains of the power stage measurement are known.
const app::LinearScaling kVoltageScaling = { 5.0F / 4096.0F, 0.0F };
const app::LinearScaling kCurrentScaling = { 5.0F / 4096.0F, 0.0F };

// Setpoint slew rates as in Project A (constants [100, -100] at both loops).
const app::ControlParameters kControlParameters = {
    100.0F, 100.0F,     // voltage: V/s rise, fall
    100.0F, 100.0F      // current: A/s rise, fall
};

hal::GpioOutput led1(DEVICE_GPIO_PIN_LED1);
app::Blinker blinker(led1, kBlinkTicks);
hal::CpuTimer tickTimer(CPUTIMER0_BASE, DEVICE_SYSCLK_FREQ);

hal::EpwmTimeBase powerStageTimeBase(EPWM4_BASE);
hal::EpwmTimeBase voltageTriggerTimeBase(EPWM5_BASE);
hal::PowerStagePwm powerStagePwm(EPWM4_BASE, kPwmPeriodCounts);
hal::TripZone powerStageTrip(EPWM4_BASE);

hal::Adc adcA(ADCA_BASE);       // output current, ADCINA0, triggered by ePWM4
hal::Adc adcB(ADCB_BASE);       // output voltage, ADCINB0, triggered by ePWM5
hal::AdcInput currentInput(ADCARESULT_BASE, ADC_SOC_NUMBER0);
hal::AdcInput voltageInput(ADCBRESULT_BASE, ADC_SOC_NUMBER0);

ctrl::VoltageLoop voltageLoop;
ctrl::CurrentLoop currentLoop;
app::ControlTask controlTask(voltageInput, currentInput, kVoltageScaling, kCurrentScaling,
                             kControlParameters, voltageLoop, currentLoop,
                             powerStagePwm, powerStageTrip);

__interrupt void cpuTimer0Isr(void)
{
    blinker.tick();
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

// Current control ISR, 10 kHz (PIE 1.1): end of the current conversion on
// ADC A (ePWM4 SOCA). Current loop -> PWM duty cycle.
__interrupt void currentControlIsr(void)
{
    controlTask.stepCurrentLoop();
    if (adcA.acknowledgeInterrupt(ADC_INT_NUMBER1)) {
        ++controlStatus.currentAdcOverflowCount;
    }
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

// Voltage control ISR, 10 kHz (PIE 1.2): end of the voltage conversion on
// ADC B (ePWM5 SOCA, 0.5 us after ADC A). Voltage loop -> current reference,
// used by the current ISR of the next period.
__interrupt void voltageControlIsr(void)
{
    controlTask.stepVoltageLoop();
    if (adcB.acknowledgeInterrupt(ADC_INT_NUMBER1)) {
        ++controlStatus.voltageAdcOverflowCount;
    }
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

void initBoardGpio()
{
    Device_initGPIO();

    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);

    // LED2 belongs to CPU2 (sign of life of the communication core).
    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(DEVICE_GPIO_PIN_LED2, GPIO_CORE_CPU2);

    // Power stage: ePWM4A (high side) on GPIO6, ePWM4B (low side) on GPIO7.
    GPIO_setPadConfig(6U, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_6_EPWM4A);
    GPIO_setPadConfig(7U, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_7_EPWM4B);
}

void initPowerStagePwm()
{
    // Stop all time bases, so ePWM4 and ePWM5 start in sync.
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    powerStageTimeBase.configureUpDown(kPwmPeriodCounts);
    powerStageTimeBase.makeSyncSource();
    powerStageTimeBase.enableAdcTriggerA(EPWM_SOC_TBCTR_PERIOD, kAdcTriggerPrescale);
    powerStageTrip.configure();             // outputs off until enabled
    powerStagePwm.configureOutputs(kDeadBandCounts);

    voltageTriggerTimeBase.configureUpDown(kPwmPeriodCounts);
    voltageTriggerTimeBase.followSync(kVoltageTriggerPhase, EPWM_COUNT_MODE_DOWN_AFTER_SYNC);
    voltageTriggerTimeBase.enableAdcTriggerA(EPWM_SOC_TBCTR_PERIOD, kAdcTriggerPrescale);

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
}

void initAdcs()
{
    adcA.powerUp();
    adcB.powerUp();
    DEVICE_DELAY_US(1000);

    adcA.configureSoc(ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM4_SOCA, ADC_CH_ADCIN0, kAdcSampleWindow);
    adcB.configureSoc(ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM5_SOCA, ADC_CH_ADCIN0, kAdcSampleWindow);
    adcA.enableEndOfConversionInterrupt(ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    adcB.enableEndOfConversionInterrupt(ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
}

void applyCommand()
{
    app::ControlCommand command;
    command.outputEnable = controlCommand.outputEnable;
    command.mode = (controlCommand.mode == ctrl::VoltageLoopInputs::Mode::kOpenLoop)
        ? ctrl::VoltageLoopInputs::Mode::kOpenLoop
        : ctrl::VoltageLoopInputs::Mode::kClosedLoop;
    command.voltageSetpoint = controlCommand.voltageSetpoint;
    command.currentSetpoint = controlCommand.currentSetpoint;
    controlTask.setCommand(command);
}

void updateStatus()
{
    controlStatus.outputEnabled = controlTask.isOutputEnabled();
    controlStatus.measurements = controlTask.measurements();
    controlStatus.voltageLoop = controlTask.voltageLoopOutputs();
    controlStatus.currentLoop = controlTask.currentLoopOutputs();
    controlStatus.voltageIsrCount = controlTask.voltageStepCount();
    controlStatus.currentIsrCount = controlTask.currentStepCount();
}

} // namespace

int main()
{
    Device_init();

#if defined(_FLASH) && defined(_STANDALONE)
    // Without a debugger nobody else starts CPU2: boot it from its flash.
    Device_bootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH);
#endif

    initBoardGpio();

    // CPU2 waits for this flag before it touches any peripheral that CPU1
    // hands over above.
    ipc::syncCores(IPC_CPU1_L_CPU2_R, shared_types::kIpcFlagSync);

    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_register(INT_TIMER0, &cpuTimer0Isr);
    Interrupt_register(INT_ADCA1, &currentControlIsr);
    Interrupt_register(INT_ADCB1, &voltageControlIsr);

    initAdcs();
    initPowerStagePwm();

    tickTimer.configurePeriodic(kTickPeriodUs);
    Interrupt_enable(INT_TIMER0);
    Interrupt_enable(INT_ADCA1);
    Interrupt_enable(INT_ADCB1);
    tickTimer.start();

    EINT;
    ERTM;

    for (;;) {
        applyCommand();
        updateStatus();
    }
}
