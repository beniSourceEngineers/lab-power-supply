#ifndef HAL_TRIPZONE_HPP
#define HAL_TRIPZONE_HPP

#include <stdint.h>

#include "driverlib.h"
#include "hal/ITripZone.hpp"

namespace hal {

// One-shot trip zone of one ePWM module. While tripped, xA and xB are forced
// low, after dead band, so the power stage is off.
class TripZone : public ITripZone {
public:
    explicit TripZone(uint32_t base);

    // Trip action xA/xB low; trips immediately, so the outputs start off.
    void configure();

    // Hardware backstop: trip when tripInput (an ePWM X-BAR TRIPx, e.g. from a
    // CMPSS) goes high. Uses digital compare A: DCAH = tripInput, DCAEVT1 on
    // DCAH high, DCAEVT1 as one-shot source.
    void enableDigitalCompareTrip(EPWM_DigitalCompareTripInput tripInput);

    virtual void forceTrip();
    virtual void clearTrip();
    virtual bool isTripped() const;

private:
    const uint32_t base_;
};

} // namespace hal

#endif
