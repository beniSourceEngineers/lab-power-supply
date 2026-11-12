#ifndef HAL_CMPSS_HPP
#define HAL_CMPSS_HPP

#include <stdint.h>

#include "driverlib.h"

namespace hal {

// High comparator of one CMPSS module as a hardware over-limit trip: the pin
// voltage is compared with the internal DAC, filtered, and routed to an ePWM
// X-BAR TRIP input. TripZone::enableDigitalCompareTrip() then trips the PWM.
// Not used yet: the measurements of the LaunchPad set-up (ADCINA0/ADCINB0) are
// not CMPSS inputs. Only the composition root uses it, so it has no interface.
class Cmpss {
public:
    explicit Cmpss(uint32_t base);

    // Trips when the input is above dacThreshold (0..4095 of VDDA). The digital
    // filter samples every (samplePrescale + 1) SYSCLK cycles and needs
    // filterThreshold of the last filterWindow samples high (filterThreshold >
    // filterWindow / 2).
    void configureHighTrip(uint16_t dacThreshold, uint16_t samplePrescale,
                           uint16_t filterWindow, uint16_t filterThreshold);

    // Route the filtered high trip to trip (XBAR_TRIP4, ...) through muxConfig,
    // e.g. XBAR_EPWM_MUX00_CMPSS1_CTRIPH with mux XBAR_MUX00.
    void routeHighTrip(XBAR_TripNum trip, XBAR_EPWMMuxConfig muxConfig, uint32_t mux);

private:
    const uint32_t base_;
};

} // namespace hal

#endif
