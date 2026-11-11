#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/TripZone.hpp"

using fake::call;
using ::testing::ElementsAre;

namespace {
const uint32_t kBase = 0x4300U;     // EPWM4_BASE
}

class TripZoneTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        fake::driverlibCalls().clear();
        fake::resetPeripherals();
    }
};

TEST_F(TripZoneTest, ConfigureForcesOutputsLowAndTrips)
{
    hal::TripZone trip(kBase);
    trip.configure();

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_setTripZoneAction", {kBase, EPWM_TZ_ACTION_EVENT_TZA, EPWM_TZ_ACTION_LOW}),
        call("EPWM_setTripZoneAction", {kBase, EPWM_TZ_ACTION_EVENT_TZB, EPWM_TZ_ACTION_LOW}),
        call("EPWM_forceTripZoneEvent", {kBase, EPWM_TZ_FORCE_EVENT_OST})));
}

TEST_F(TripZoneTest, ClearClearsOneShotAndInterruptFlag)
{
    hal::TripZone trip(kBase);
    trip.clearTrip();

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_clearTripZoneFlag", {kBase, EPWM_TZ_INTERRUPT | EPWM_TZ_FLAG_OST})));
}

TEST_F(TripZoneTest, IsTrippedReadsOneShotFlag)
{
    const hal::TripZone trip(kBase);
    EXPECT_FALSE(trip.isTripped());

    fake::setTripZoneFlags(kBase, EPWM_TZ_INTERRUPT);
    EXPECT_FALSE(trip.isTripped());

    fake::setTripZoneFlags(kBase, EPWM_TZ_INTERRUPT | EPWM_TZ_FLAG_OST);
    EXPECT_TRUE(trip.isTripped());
}

TEST_F(TripZoneTest, DigitalCompareTripOnTripInputHigh)
{
    hal::TripZone trip(kBase);
    trip.enableDigitalCompareTrip(EPWM_DC_TRIP_TRIPIN4);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_selectDigitalCompareTripInput", {kBase, EPWM_DC_TRIP_TRIPIN4, EPWM_DC_TYPE_DCAH}),
        call("EPWM_setTripZoneDigitalCompareEventCondition",
             {kBase, EPWM_TZ_DC_OUTPUT_A1, EPWM_TZ_EVENT_DCXH_HIGH}),
        call("EPWM_setDigitalCompareEventSource",
             {kBase, EPWM_DC_MODULE_A, EPWM_DC_EVENT_1, EPWM_DC_EVENT_SOURCE_ORIG_SIGNAL}),
        call("EPWM_enableTripZoneSignals", {kBase, EPWM_TZ_SIGNAL_DCAEVT1})));
}
