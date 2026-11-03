#ifndef HAL_CPUTIMER_HPP
#define HAL_CPUTIMER_HPP

#include <stdint.h>

namespace hal {

// Periodic CPU timer (CPUTIMER0/1/2) that raises its interrupt every period.
// The ISR itself is registered by the composition root.
class CpuTimer {
public:
    CpuTimer(uint32_t base, uint32_t sysClkHz);

    // Stops the timer and sets it up for an interrupt every periodUs.
    void configurePeriodic(uint32_t periodUs);
    void start();

private:
    const uint32_t base_;
    const uint32_t sysClkHz_;
};

} // namespace hal

#endif
