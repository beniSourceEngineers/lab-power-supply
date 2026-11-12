#include "hal/EpwmTimeBase.hpp"

namespace hal {

EpwmTimeBase::EpwmTimeBase(uint32_t base)
    : base_(base)
{
}

void EpwmTimeBase::configureUpDown(uint16_t periodCounts)
{
    EPWM_setClockPrescaler(base_, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBasePeriod(base_, periodCounts);
    EPWM_setTimeBaseCounter(base_, 0U);
    EPWM_setTimeBaseCounterMode(base_, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setEmulationMode(base_, EPWM_EMULATION_FREE_RUN);
}

void EpwmTimeBase::makeSyncSource()
{
    EPWM_disablePhaseShiftLoad(base_);
    EPWM_setPhaseShift(base_, 0U);
    EPWM_setSyncOutPulseMode(base_, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO);
}

void EpwmTimeBase::followSync(uint16_t phaseCounts, EPWM_SyncCountMode directionAfterSync)
{
    EPWM_setPhaseShift(base_, phaseCounts);
    EPWM_setCountModeAfterSync(base_, directionAfterSync);
    EPWM_enablePhaseShiftLoad(base_);
    EPWM_setSyncOutPulseMode(base_, EPWM_SYNC_OUT_PULSE_ON_EPWMxSYNCIN);
}

void EpwmTimeBase::enableAdcTriggerA(EPWM_ADCStartOfConversionSource source,
                                     uint16_t eventPrescale)
{
    EPWM_setADCTriggerSource(base_, EPWM_SOC_A, source);
    EPWM_setADCTriggerEventPrescale(base_, EPWM_SOC_A, eventPrescale);
    EPWM_enableADCTrigger(base_, EPWM_SOC_A);
}

} // namespace hal
