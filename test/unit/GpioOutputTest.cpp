#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/GpioOutput.hpp"

using ::testing::ElementsAre;

class GpioOutputTest : public ::testing::Test {
protected:
    void SetUp() override { fake::driverlibCalls().clear(); }
};

TEST_F(GpioOutputTest, WriteHighSetsPin)
{
    hal::GpioOutput pin(31U);
    pin.write(true);
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre("GPIO_writePin(31, 1)"));
}

TEST_F(GpioOutputTest, WriteLowClearsPin)
{
    hal::GpioOutput pin(34U);
    pin.write(false);
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre("GPIO_writePin(34, 0)"));
}

TEST_F(GpioOutputTest, ToggleTogglesPin)
{
    hal::GpioOutput pin(31U);
    pin.toggle();
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre("GPIO_togglePin(31)"));
}
