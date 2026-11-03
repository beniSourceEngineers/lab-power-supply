// CPU2 composition root: communication core (AGENTS.md §3.4, §5).
// GPIO pad configuration and peripheral ownership are done by CPU1.

#include <stdint.h>

extern "C" {
#include "device.h"
}

#include "app/Blinker.hpp"
#include "hal/CpuTimer.hpp"
#include "hal/GpioOutput.hpp"

namespace {

const uint32_t kTickPeriodUs = 1000UL;     // 1 kHz system tick
const uint16_t kBlinkTicks = 250U;         // LED2 toggles every 250 ms

hal::GpioOutput led2(DEVICE_GPIO_PIN_LED2);
app::Blinker blinker(led2, kBlinkTicks);
hal::CpuTimer tickTimer(CPUTIMER0_BASE, DEVICE_SYSCLK_FREQ);

__interrupt void cpuTimer0Isr(void)
{
    blinker.tick();
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

} // namespace

int main()
{
    Device_init();

    // Wait until CPU1 has configured the GPIOs and handed LED2 over.
    IPC_sync(IPC_CPU2_L_CPU1_R, IPC_FLAG31);

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
