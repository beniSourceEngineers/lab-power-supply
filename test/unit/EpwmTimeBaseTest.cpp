#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/EpwmTimeBase.hpp"

using fake::call;
using ::testing::ElementsAre;

namespace {
const uint32_t kBase = 0x4400U;     // EPWM5_BASE
}

class EpwmTimeBaseTest : public ::testing::Test {
protected:
    void SetUp() override { fake::driverlibCalls().clear(); }
};

TEST_F(EpwmTimeBaseTest, ConfigureUpDownSetsPeriodWithoutPrescaler)
{
    hal::EpwmTimeBase timeBase(kBase);
    timeBase.configureUpDown(1000U);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_setClockPrescaler", {kBase, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1}),
        call("EPWM_setTimeBasePeriod", {kBase, 1000U}),
        call("EPWM_setTimeBaseCounter", {kBase, 0U}),
        call("EPWM_setTimeBaseCounterMode", {kBase, EPWM_COUNTER_MODE_UP_DOWN}),
        call("EPWM_setEmulationMode", {kBase, EPWM_EMULATION_FREE_RUN})));
}

TEST_F(EpwmTimeBaseTest, SyncSourceIgnoresSyncInAndSendsAtZero)
{
    hal::EpwmTimeBase timeBase(kBase);
    timeBase.makeSyncSource();

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_disablePhaseShiftLoad", {kBase}),
        call("EPWM_setPhaseShift", {kBase, 0U}),
        call("EPWM_setSyncOutPulseMode", {kBase, EPWM_SYNC_OUT_PULSE_ON_COUNTER_ZERO})));
}

TEST_F(EpwmTimeBaseTest, FollowerLoadsPhaseAndPassesSyncOn)
{
    hal::EpwmTimeBase timeBase(kBase);
    timeBase.followSync(50U, EPWM_COUNT_MODE_DOWN_AFTER_SYNC);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_setPhaseShift", {kBase, 50U}),
        call("EPWM_setCountModeAfterSync", {kBase, EPWM_COUNT_MODE_DOWN_AFTER_SYNC}),
        call("EPWM_enablePhaseShiftLoad", {kBase}),
        call("EPWM_setSyncOutPulseMode", {kBase, EPWM_SYNC_OUT_PULSE_ON_EPWMxSYNCIN})));
}

TEST_F(EpwmTimeBaseTest, AdcTriggerAWithPrescale)
{
    hal::EpwmTimeBase timeBase(kBase);
    timeBase.enableAdcTriggerA(EPWM_SOC_TBCTR_PERIOD, 5U);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_setADCTriggerSource", {kBase, EPWM_SOC_A, EPWM_SOC_TBCTR_PERIOD}),
        call("EPWM_setADCTriggerEventPrescale", {kBase, EPWM_SOC_A, 5U}),
        call("EPWM_enableADCTrigger", {kBase, EPWM_SOC_A})));
}
