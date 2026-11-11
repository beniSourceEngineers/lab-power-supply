#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockAnalogInput.hpp"
#include "MockPwm.hpp"
#include "MockTripZone.hpp"
#include "app/OpenLoopTask.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

class OpenLoopTaskTest : public ::testing::Test {
protected:
    OpenLoopTaskTest()
        : task(voltage, current, kVoltageScaling, kCurrentScaling, pwm, trip)
    {
        ON_CALL(voltage, readRaw()).WillByDefault(Return(0U));
        ON_CALL(current, readRaw()).WillByDefault(Return(0U));
        ON_CALL(trip, isTripped()).WillByDefault(Return(false));
    }

    static const app::LinearScaling kVoltageScaling;
    static const app::LinearScaling kCurrentScaling;

    NiceMock<MockAnalogInput> voltage;
    NiceMock<MockAnalogInput> current;
    NiceMock<MockPwm> pwm;
    NiceMock<MockTripZone> trip;
    app::OpenLoopTask task;
};

const app::LinearScaling OpenLoopTaskTest::kVoltageScaling = { 0.01F, 0.0F };
const app::LinearScaling OpenLoopTaskTest::kCurrentScaling = { 0.001F, -1.0F };

TEST_F(OpenLoopTaskTest, StepScalesMeasurementsAndCounts)
{
    EXPECT_CALL(voltage, readRaw()).WillOnce(Return(1200U));
    EXPECT_CALL(current, readRaw()).WillOnce(Return(3000U));

    task.step();

    EXPECT_FLOAT_EQ(task.measurements().outputVoltage, 12.0F);
    EXPECT_FLOAT_EQ(task.measurements().outputCurrent, 2.0F);
    EXPECT_EQ(task.stepCount(), 1U);
}

TEST_F(OpenLoopTaskTest, OutputOffAtStartAppliesZeroDuty)
{
    EXPECT_CALL(pwm, setDuty(0.0F));
    task.setCommand(false, 0.4F);
    task.step();
    EXPECT_FALSE(task.isOutputEnabled());
}

TEST_F(OpenLoopTaskTest, SwitchOnClearsTripAndAppliesDuty)
{
    {
        InSequence sequence;
        EXPECT_CALL(trip, clearTrip());
        EXPECT_CALL(pwm, setDuty(0.4F));
    }
    task.setCommand(true, 0.4F);
    task.step();
    EXPECT_TRUE(task.isOutputEnabled());
}

TEST_F(OpenLoopTaskTest, DutyChangeWhileOnDoesNotTouchTrip)
{
    task.setCommand(true, 0.4F);

    EXPECT_CALL(trip, clearTrip()).Times(0);
    EXPECT_CALL(trip, forceTrip()).Times(0);
    EXPECT_CALL(pwm, setDuty(0.6F));
    task.setCommand(true, 0.6F);
    task.step();
}

TEST_F(OpenLoopTaskTest, SwitchOffForcesTripAndZeroDuty)
{
    task.setCommand(true, 0.4F);

    EXPECT_CALL(trip, forceTrip());
    EXPECT_CALL(pwm, setDuty(0.0F));
    task.setCommand(false, 0.4F);
    task.step();
    EXPECT_FALSE(task.isOutputEnabled());
}

TEST_F(OpenLoopTaskTest, HardwareTripLatchesOffUntilOffOnCommand)
{
    task.setCommand(true, 0.4F);
    ON_CALL(trip, isTripped()).WillByDefault(Return(true));

    EXPECT_CALL(pwm, setDuty(0.0F)).Times(2);
    task.step();
    task.setCommand(true, 0.4F);        // same command: stays off
    task.step();
    EXPECT_FALSE(task.isOutputEnabled());

    ON_CALL(trip, isTripped()).WillByDefault(Return(false));
    task.setCommand(false, 0.4F);
    EXPECT_CALL(trip, clearTrip());
    EXPECT_CALL(pwm, setDuty(0.4F));
    task.setCommand(true, 0.4F);
    task.step();
    EXPECT_TRUE(task.isOutputEnabled());
}
