#include "hal/CpuTimer.hpp"

#include "driverlib.h"

namespace hal {

CpuTimer::CpuTimer(uint32_t base, uint32_t sysClkHz)
    : base_(base)
    , sysClkHz_(sysClkHz)
{
}

void CpuTimer::configurePeriodic(uint32_t periodUs)
{
    const uint32_t ticksPerUs = sysClkHz_ / 1000000UL;

    CPUTimer_stopTimer(base_);
    CPUTimer_setPeriod(base_, ticksPerUs * periodUs - 1UL);
    CPUTimer_setPreScaler(base_, 0U);
    CPUTimer_reloadTimerCounter(base_);
    CPUTimer_setEmulationMode(base_, CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_enableInterrupt(base_);
}

void CpuTimer::start()
{
    CPUTimer_startTimer(base_);
}

} // namespace hal
