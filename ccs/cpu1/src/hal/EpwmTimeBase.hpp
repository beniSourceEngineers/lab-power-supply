#ifndef HAL_EPWMTIMEBASE_HPP
#define HAL_EPWMTIMEBASE_HPP

#include <stdint.h>

#include "driverlib.h"

namespace hal {

// Time base of one ePWM module in up-down count mode, its synchronisation and
// its ADC start-of-conversion trigger A. The outputs are set up separately
// (PowerStagePwm). Configure all modules while TBCLKSYNC is off, so they start
// together when the composition root enables it.
class EpwmTimeBase {
public:
    explicit EpwmTimeBase(uint32_t base);

    // Up-down count, TBCLK = EPWMCLK (no prescaler), counter at 0, free run
    // while the debugger halts the CPU. PWM frequency = TBCLK / (2 * periodCounts).
    void configureUpDown(uint16_t periodCounts);

    // Master of a sync chain: ignores SYNCIN, sends SYNCOUT at counter zero.
    void makeSyncSource();

    // Follower: loads phaseCounts on SYNCIN and counts in the given direction
    // afterwards; passes SYNCIN on to the next module.
    void followSync(uint16_t phaseCounts, EPWM_SyncCountMode directionAfterSync);

    // SOCA at the given counter event, every eventPrescale-th event (1..15).
    void enableAdcTriggerA(EPWM_ADCStartOfConversionSource source, uint16_t eventPrescale);

    uint32_t base() const { return base_; }

private:
    const uint32_t base_;
};

} // namespace hal

#endif
