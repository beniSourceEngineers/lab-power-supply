#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/Cmpss.hpp"

using fake::call;
using ::testing::ElementsAre;

namespace {
const uint32_t kBase = 0x5C80U;     // CMPSS1_BASE
}

class CmpssTest : public ::testing::Test {
protected:
    void SetUp() override { fake::driverlibCalls().clear(); }
};

TEST_F(CmpssTest, HighTripComparesWithDacThroughFilter)
{
    hal::Cmpss cmpss(kBase);
    cmpss.configureHighTrip(3000U, 0U, 31U, 20U);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("CMPSS_enableModule", {kBase}),
        call("CMPSS_configHighComparator", {kBase, CMPSS_INSRC_DAC}),
        call("CMPSS_configDAC", {kBase, CMPSS_DACREF_VDDA | CMPSS_DACVAL_SYSCLK | CMPSS_DACSRC_SHDW}),
        call("CMPSS_setDACValueHigh", {kBase, 3000U}),
        call("CMPSS_configFilterHigh", {kBase, 0U, 31U, 20U}),
        call("CMPSS_initFilterHigh", {kBase}),
        call("CMPSS_configOutputsHigh", {kBase, CMPSS_TRIP_FILTER | CMPSS_TRIPOUT_FILTER})));
}

TEST_F(CmpssTest, RouteHighTripToEpwmXbar)
{
    hal::Cmpss cmpss(kBase);
    cmpss.routeHighTrip(XBAR_TRIP4, XBAR_EPWM_MUX00_CMPSS1_CTRIPH, XBAR_MUX00);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("XBAR_setEPWMMuxConfig", {XBAR_TRIP4, XBAR_EPWM_MUX00_CMPSS1_CTRIPH}),
        call("XBAR_enableEPWMMux", {XBAR_TRIP4, XBAR_MUX00})));
}
