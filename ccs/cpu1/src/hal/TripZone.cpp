#include "hal/TripZone.hpp"

namespace hal {

TripZone::TripZone(uint32_t base)
    : base_(base)
{
}

void TripZone::configure()
{
    EPWM_setTripZoneAction(base_, EPWM_TZ_ACTION_EVENT_TZA, EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(base_, EPWM_TZ_ACTION_EVENT_TZB, EPWM_TZ_ACTION_LOW);
    forceTrip();
}

void TripZone::enableDigitalCompareTrip(EPWM_DigitalCompareTripInput tripInput)
{
    EPWM_selectDigitalCompareTripInput(base_, tripInput, EPWM_DC_TYPE_DCAH);
    EPWM_setTripZoneDigitalCompareEventCondition(base_, EPWM_TZ_DC_OUTPUT_A1,
                                                 EPWM_TZ_EVENT_DCXH_HIGH);
    EPWM_setDigitalCompareEventSource(base_, EPWM_DC_MODULE_A, EPWM_DC_EVENT_1,
                                      EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL);
    EPWM_enableTripZoneSignals(base_, EPWM_TZ_SIGNAL_DCAEVT1);
}

void TripZone::forceTrip()
{
    EPWM_forceTripZoneEvent(base_, EPWM_TZ_FORCE_EVENT_OST);
}

void TripZone::clearTrip()
{
    EPWM_clearTripZoneFlag(base_, EPWM_TZ_INTERRUPT | EPWM_TZ_FLAG_OST);
}

bool TripZone::isTripped() const
{
    return (EPWM_getTripZoneFlagStatus(base_) & EPWM_TZ_FLAG_OST) != 0U;
}

} // namespace hal
