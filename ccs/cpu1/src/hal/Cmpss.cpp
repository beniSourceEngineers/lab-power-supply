#include "hal/Cmpss.hpp"

namespace hal {

Cmpss::Cmpss(uint32_t base)
    : base_(base)
{
}

void Cmpss::configureHighTrip(uint16_t dacThreshold, uint16_t samplePrescale,
                              uint16_t filterWindow, uint16_t filterThreshold)
{
    CMPSS_enableModule(base_);
    CMPSS_configHighComparator(base_, CMPSS_INSRC_DAC);
    CMPSS_configDAC(base_, CMPSS_DACREF_VDDA | CMPSS_DACVAL_SYSCLK | CMPSS_DACSRC_SHDW);
    CMPSS_setDACValueHigh(base_, dacThreshold);
    CMPSS_configFilterHigh(base_, samplePrescale, filterWindow, filterThreshold);
    CMPSS_initFilterHigh(base_);
    CMPSS_configOutputsHigh(base_, CMPSS_TRIP_FILTER | CMPSS_TRIPOUT_FILTER);
}

void Cmpss::routeHighTrip(XBAR_TripNum trip, XBAR_EPWMMuxConfig muxConfig, uint32_t mux)
{
    XBAR_setEPWMMuxConfig(trip, muxConfig);
    XBAR_enableEPWMMux(trip, mux);
}

} // namespace hal
