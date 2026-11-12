#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "driverlib.h"
#include "hal/PowerStagePwm.hpp"

using fake::call;
using ::testing::ElementsAre;

namespace {
const uint32_t kBase = 0x4300U;     // EPWM4_BASE
const uint16_t kPeriod = 1000U;
}

class PowerStagePwmTest : public ::testing::Test {
protected:
    void SetUp() override { fake::driverlibCalls().clear(); }

    std::string cmpa(uint32_t counts)
    {
        return call("EPWM_setCounterCompareValue", {kBase, EPWM_COUNTER_COMPARE_A, counts});
    }
};

TEST_F(PowerStagePwmTest, ConfigureOutputsComplementaryWithDeadBandAndZeroDuty)
{
    hal::PowerStagePwm pwm(kBase, kPeriod);
    pwm.configureOutputs(20U);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("EPWM_setCounterCompareShadowLoadMode",
             {kBase, EPWM_COUNTER_COMPARE_A, EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD}),
        cmpa(0U),
        call("EPWM_setActionQualifierAction",
             {kBase, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA}),
        call("EPWM_setActionQualifierAction",
             {kBase, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA}),
        call("EPWM_setRisingEdgeDeadBandDelayInput", {kBase, EPWM_DB_INPUT_EPWMA}),
        call("EPWM_setFallingEdgeDeadBandDelayInput", {kBase, EPWM_DB_INPUT_EPWMA}),
        call("EPWM_setDeadBandDelayPolarity", {kBase, EPWM_DB_RED, EPWM_DB_POLARITY_ACTIVE_HIGH}),
        call("EPWM_setDeadBandDelayPolarity", {kBase, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW}),
        call("EPWM_setDeadBandCounterClock", {kBase, EPWM_DB_COUNTER_CLOCK_HALF_CYCLE}),
        call("EPWM_setRisingEdgeDelayCount", {kBase, 20U}),
        call("EPWM_setFallingEdgeDelayCount", {kBase, 20U}),
        call("EPWM_setDeadBandDelayMode", {kBase, EPWM_DB_RED, 1U}),
        call("EPWM_setDeadBandDelayMode", {kBase, EPWM_DB_FED, 1U})));
}

TEST_F(PowerStagePwmTest, DutyIsCompareOverPeriodRounded)
{
    hal::PowerStagePwm pwm(kBase, kPeriod);
    pwm.setDuty(0.25F);
    pwm.setDuty(0.3337F);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(cmpa(250U), cmpa(334U)));
}

TEST_F(PowerStagePwmTest, DutyIsClampedToZeroAndOne)
{
    hal::PowerStagePwm pwm(kBase, kPeriod);
    pwm.setDuty(-0.1F);
    pwm.setDuty(1.5F);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(cmpa(0U), cmpa(kPeriod)));
}
