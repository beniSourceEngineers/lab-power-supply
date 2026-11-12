#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/Adc.hpp"

using fake::call;
using ::testing::ElementsAre;

namespace {
const uint32_t kBase = 0x7480U;         // ADCB_BASE
}

class AdcTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        fake::driverlibCalls().clear();
        fake::resetPeripherals();
    }
};

TEST_F(AdcTest, PowerUpSets12BitSingleEndedWithEndOfConversionPulse)
{
    hal::Adc adc(kBase);
    adc.powerUp();

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("ADC_setPrescaler", {kBase, ADC_CLK_DIV_4_0}),
        call("ADC_setMode", {kBase, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED}),
        call("ADC_setInterruptPulseMode", {kBase, ADC_PULSE_END_OF_CONV}),
        call("ADC_enableConverter", {kBase})));
}

TEST_F(AdcTest, ConfigureSocPassesTriggerChannelAndWindow)
{
    hal::Adc adc(kBase);
    adc.configureSoc(ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM5_SOCA, ADC_CH_ADCIN0, 16U);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("ADC_setupSOC", {kBase, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM5_SOCA, ADC_CH_ADCIN0, 16U})));
}

TEST_F(AdcTest, EndOfConversionInterruptIsContinuousAndStartsCleared)
{
    hal::Adc adc(kBase);
    adc.enableEndOfConversionInterrupt(ADC_INT_NUMBER1, ADC_SOC_NUMBER0);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("ADC_setInterruptSource", {kBase, ADC_INT_NUMBER1, ADC_SOC_NUMBER0}),
        call("ADC_enableContinuousMode", {kBase, ADC_INT_NUMBER1}),
        call("ADC_enableInterrupt", {kBase, ADC_INT_NUMBER1}),
        call("ADC_clearInterruptStatus", {kBase, ADC_INT_NUMBER1})));
}

TEST_F(AdcTest, AcknowledgeWithoutOverflowOnlyClearsFlag)
{
    hal::Adc adc(kBase);
    EXPECT_FALSE(adc.acknowledgeInterrupt(ADC_INT_NUMBER1));
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("ADC_clearInterruptStatus", {kBase, ADC_INT_NUMBER1})));
}

TEST_F(AdcTest, AcknowledgeReportsAndClearsOverflow)
{
    hal::Adc adc(kBase);
    fake::setAdcInterruptOverflow(true);

    EXPECT_TRUE(adc.acknowledgeInterrupt(ADC_INT_NUMBER1));
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        call("ADC_clearInterruptOverflowStatus", {kBase, ADC_INT_NUMBER1}),
        call("ADC_clearInterruptStatus", {kBase, ADC_INT_NUMBER1})));
}
