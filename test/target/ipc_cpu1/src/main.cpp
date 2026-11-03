// IPC target test, CPU1 side: writes the pattern mailbox at 10 kHz from a timer
// ISR, reads CPU2's report, pops CPU2's events and sends commands to CPU2.
// The verdict is in ipcTestResult and on LED1, see test/target/README.md.

#include <stdint.h>

extern "C" {
#include "device.h"
}

#include "IpcTestChannels.hpp"
#include "hal/CpuTimer.hpp"
#include "hal/GpioOutput.hpp"
#include "ipc/CommandLink.hpp"
#include "ipc/SyncCores.hpp"
#include "shared_types/IpcFlags.hpp"

using namespace ipctest;

// One object per section, see ipc/MsgRamSections.hpp.
#pragma DATA_SECTION(IPC_SECTION_CPU1_TO_CPU2)
Cpu1ToCpu2Channels cpu1ToCpu2;
#pragma DATA_SECTION(IPC_SECTION_CPU2_TO_CPU1)
Cpu2ToCpu1Channels cpu2ToCpu1;

struct Verdict {
    enum Type { kRunning = 0, kPassed = 1, kFailed = 2 };
};

struct Result {
    volatile uint16_t verdict;
    volatile uint32_t addressesOk;
    volatile uint32_t patternsPosted;
    volatile uint32_t eventsReceived;
    volatile uint32_t eventErrors;
    volatile uint32_t commandsOk;
    volatile uint32_t commandErrors;
    Cpu2Report cpu2;
};

// Watch this in the CCS expressions view. Not static, so it stays visible.
Result ipcTestResult;

namespace {

const uint32_t kPatternPeriodUs = 100UL;   // 10 kHz, the rate of the control ISR
const uint32_t kRequiredCount = 100000UL;  // of each kind of exchange for a pass

// LED1 toggle periods, in timer ticks.
const uint32_t kBlinkRunningTicks = 1000UL;    // 100 ms
const uint32_t kBlinkPassedTicks = 10000UL;    // 1 s

hal::GpioOutput led1(DEVICE_GPIO_PIN_LED1);
hal::CpuTimer timer(CPUTIMER0_BASE, DEVICE_SYSCLK_FREQ);

PatternMailbox patternMailbox(cpu1ToCpu2.pattern, IPC_CPU1_L_CPU2_R);
ReportMailbox reportMailbox(cpu2ToCpu1.report, IPC_CPU1_L_CPU2_R);
EventQueue eventQueue(cpu2ToCpu1.eventsProducer, cpu1ToCpu2.eventsConsumer, IPC_CPU1_L_CPU2_R);
ipc::CommandLink commandLink(IPC_CPU1_L_CPU2_R, kFlagCommand);

uint32_t blinkTicks = 0UL;

void updateLed()
{
    const uint16_t verdict = ipcTestResult.verdict;
    if (verdict == Verdict::kFailed) {
        led1.write(false);   // the LaunchPad LEDs are active low: steady on
        return;
    }
    const uint32_t period = (verdict == Verdict::kPassed) ? kBlinkPassedTicks : kBlinkRunningTicks;
    if (++blinkTicks >= period) {
        blinkTicks = 0UL;
        led1.toggle();
    }
}

__interrupt void timerIsr(void)
{
    const uint32_t counter = ipcTestResult.patternsPosted + 1UL;
    patternMailbox.post(makePattern(counter));
    ipcTestResult.patternsPosted = counter;

    updateLed();
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

void readReport()
{
    Cpu2Report report;
    if (reportMailbox.tryRead(report)) {
        ipcTestResult.cpu2 = report;
    }
}

void receiveEvents()
{
    static uint32_t expected = 0UL;
    Event event;
    while (eventQueue.pop(event)) {
        if ((event.value != expected) || (event.inverted != ~expected)) {
            ipcTestResult.eventErrors = ipcTestResult.eventErrors + 1UL;
        }
        expected = event.value + 1UL;
        ipcTestResult.eventsReceived = ipcTestResult.eventsReceived + 1UL;
    }
}

void exchangeCommand()
{
    static bool pending = false;
    static uint32_t value = 0UL;

    if (pending) {
        if (commandLink.isBusy()) {
            return;
        }
        if (commandLink.response() == (value + 1UL)) {
            ipcTestResult.commandsOk = ipcTestResult.commandsOk + 1UL;
        } else {
            ipcTestResult.commandErrors = ipcTestResult.commandErrors + 1UL;
        }
        pending = false;
        ++value;
    }
    pending = commandLink.send(kCommandAddOne, 0UL, value);
}

void updateVerdict()
{
    if (ipcTestResult.verdict != Verdict::kRunning) {
        return;
    }
    const bool failed = (ipcTestResult.addressesOk == 0UL)
        || (ipcTestResult.eventErrors != 0UL)
        || (ipcTestResult.commandErrors != 0UL)
        || (ipcTestResult.cpu2.patternTorn != 0UL)
        || (ipcTestResult.cpu2.patternBackwards != 0UL);
    if (failed) {
        ipcTestResult.verdict = Verdict::kFailed;
        return;
    }

    // CPU2's report arrives some time after its first read.
    const bool reportArrived = ipcTestResult.cpu2.patternReads != 0UL;
    if (reportArrived && (ipcTestResult.cpu2.addressesOk == 0UL)) {
        ipcTestResult.verdict = Verdict::kFailed;
        return;
    }

    if ((ipcTestResult.cpu2.patternReads >= kRequiredCount)
        && (ipcTestResult.cpu2.patternInterrupts >= kRequiredCount)
        && (ipcTestResult.cpu2.commandsHandled >= kRequiredCount)
        && (ipcTestResult.eventsReceived >= kRequiredCount)
        && (ipcTestResult.commandsOk >= kRequiredCount)) {
        ipcTestResult.verdict = Verdict::kPassed;
    }
}

} // namespace

int main()
{
    Device_init();
    Device_initGPIO();

    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);

    ipcTestResult.addressesOk =
        ((reinterpret_cast<uint32_t>(&cpu1ToCpu2) == ipc::kChannelsAddrCpu1ToCpu2)
         && (reinterpret_cast<uint32_t>(&cpu2ToCpu1) == ipc::kChannelsAddrCpu2ToCpu1)) ? 1UL : 0UL;

    // Own side of all channels first, then the handshake: from there on the
    // other core may read them.
    patternMailbox.initWriter();
    eventQueue.initConsumer();
    commandLink.init();
    ipc::syncCores(IPC_CPU1_L_CPU2_R, shared_types::kIpcFlagSync);

    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_register(INT_TIMER0, &timerIsr);

    timer.configurePeriodic(kPatternPeriodUs);
    Interrupt_enable(INT_TIMER0);
    timer.start();

    EINT;
    ERTM;

    for (;;) {
        readReport();
        receiveEvents();
        exchangeCommand();
        updateVerdict();
    }
}
