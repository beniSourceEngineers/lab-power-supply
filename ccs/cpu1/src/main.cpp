// CPU1 composition root: system init, boot of CPU2, peripheral hand-over,
// static objects and their wiring (AGENTS.md §3.4, §5).

#include <stdint.h>

extern "C" {
#include "device.h"
}

#include "app/Blinker.hpp"
#include "hal/CpuTimer.hpp"
#include "hal/GpioOutput.hpp"

namespace {

const uint32_t kTickPeriodUs = 1000UL;     // 1 kHz system tick
const uint16_t kBlinkTicks = 500U;         // LED1 toggles every 500 ms

hal::GpioOutput led1(DEVICE_GPIO_PIN_LED1);
app::Blinker blinker(led1, kBlinkTicks);
hal::CpuTimer tickTimer(CPUTIMER0_BASE, DEVICE_SYSCLK_FREQ);

__interrupt void cpuTimer0Isr(void)
{
    blinker.tick();
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
    IPC_sync(IPC_CPU1_L_CPU2_R, IPC_FLAG31);

    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_register(INT_TIMER0, &cpuTimer0Isr);

    tickTimer.configurePeriodic(kTickPeriodUs);
    Interrupt_enable(INT_TIMER0);
    tickTimer.start();

    EINT;
    ERTM;

    for (;;) {
    }
}
