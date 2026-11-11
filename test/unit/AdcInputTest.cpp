#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/AdcInput.hpp"

using ::testing::IsEmpty;

namespace {
const uint32_t kResultBase = 0x0B20U;   // ADCBRESULT_BASE
}

class AdcInputTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        fake::driverlibCalls().clear();
        fake::resetPeripherals();
    }
};

TEST_F(AdcInputTest, ReadsResultOfItsSoc)
{
    fake::setAdcResult(kResultBase, ADC_SOC_NUMBER0, 1234U);
    fake::setAdcResult(kResultBase, ADC_SOC_NUMBER1, 99U);
    const hal::AdcInput input(kResultBase, ADC_SOC_NUMBER0);

    EXPECT_EQ(input.readRaw(), 1234U);
    EXPECT_THAT(fake::driverlibCalls(), IsEmpty());
}
