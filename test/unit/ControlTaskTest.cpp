#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockAnalogInput.hpp"
#include "MockCurrentLoop.hpp"
#include "MockPwm.hpp"
#include "MockTripZone.hpp"
#include "MockVoltageLoop.hpp"
#include "app/ControlTask.hpp"

using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::FloatEq;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace {

typedef ctrl::VoltageLoopInputs::Mode Mode;

ctrl::ControlLoopOutputs loopOutput(float output)
{
    ctrl::ControlLoopOutputs out = {};
    out.output = output;
    return out;
}

app::ControlCommand command(bool outputEnable, Mode::Type mode = Mode::kClosedLoop)
{
    app::ControlCommand c;
    c.outputEnable = outputEnable;
    c.mode = mode;
    c.voltageSetpoint = 12.0F;
    c.currentSetpoint = 1.5F;
    return c;
}

} // namespace

class ControlTaskTest : public ::testing::Test {
protected:
    ControlTaskTest()
        : task(voltage, current, kVoltageScaling, kCurrentScaling, kParameters,
               voltageLoop, currentLoop, pwm, trip)
    {
        ON_CALL(voltage, readRaw()).WillByDefault(Return(0U));
        ON_CALL(current, readRaw()).WillByDefault(Return(0U));
        ON_CALL(trip, isTripped()).WillByDefault(Return(false));
    }

    static const app::LinearScaling kVoltageScaling;
    static const app::LinearScaling kCurrentScaling;
    static const app::ControlParameters kParameters;

    NiceMock<MockAnalogInput> voltage;
    NiceMock<MockAnalogInput> current;
    NiceMock<MockVoltageLoop> voltageLoop;
    NiceMock<MockCurrentLoop> currentLoop;
    NiceMock<MockPwm> pwm;
    NiceMock<MockTripZone> trip;
    app::ControlTask task;
};

const app::LinearScaling ControlTaskTest::kVoltageScaling = { 0.01F, 0.0F };
const app::LinearScaling ControlTaskTest::kCurrentScaling = { 0.001F, -1.0F };
const app::ControlParameters ControlTaskTest::kParameters = { 10.0F, 20.0F, 30.0F, 40.0F };

TEST_F(ControlTaskTest, VoltageStepMapsCommandAndMeasurementToVoltageLoop)
{
    task.setCommand(command(true, Mode::kOpenLoop));
    EXPECT_CALL(voltage, readRaw()).WillOnce(Return(1100U));
    EXPECT_CALL(voltageLoop, step(AllOf(
        Field(&ctrl::VoltageLoopInputs::reference, 12.0F),
        Field(&ctrl::VoltageLoopInputs::measurement, 11.0F),
        Field(&ctrl::VoltageLoopInputs::enable, true),
        Field(&ctrl::VoltageLoopInputs::slewRateRise, 10.0F),
        Field(&ctrl::VoltageLoopInputs::slewRateFall, 20.0F),
        Field(&ctrl::VoltageLoopInputs::mode, Mode::kOpenLoop),
        Field(&ctrl::VoltageLoopInputs::openLoopSetpoint, 1.5F)), _));

    task.stepVoltageLoop();

    EXPECT_FLOAT_EQ(task.measurements().outputVoltage, 11.0F);
    EXPECT_EQ(task.voltageStepCount(), 1U);
}

TEST_F(ControlTaskTest, VoltageLoopOutputIsCurrentReference)
{
    task.setCommand(command(true));
    EXPECT_CALL(voltageLoop, step(_, _)).WillOnce(SetArgReferee<1>(loopOutput(2.5F)));
    task.stepVoltageLoop();

    EXPECT_CALL(current, readRaw()).WillOnce(Return(3000U));
    EXPECT_CALL(currentLoop, step(AllOf(
        Field(&ctrl::CurrentLoopInputs::reference, 2.5F),
        Field(&ctrl::CurrentLoopInputs::measurement, FloatEq(2.0F)),
        Field(&ctrl::CurrentLoopInputs::enable, true),
        Field(&ctrl::CurrentLoopInputs::slewRateRise, 30.0F),
        Field(&ctrl::CurrentLoopInputs::slewRateFall, 40.0F)), _));
    task.stepCurrentLoop();

    EXPECT_FLOAT_EQ(task.measurements().outputCurrent, 2.0F);
    EXPECT_EQ(task.currentStepCount(), 1U);
}

TEST_F(ControlTaskTest, CurrentLoopOutputIsDutyWhenEnabled)
{
    task.setCommand(command(true));
    EXPECT_CALL(currentLoop, step(_, _)).WillOnce(SetArgReferee<1>(loopOutput(0.4F)));
    EXPECT_CALL(pwm, setDuty(0.4F));

    task.stepCurrentLoop();

    EXPECT_FLOAT_EQ(task.currentLoopOutputs().output, 0.4F);
}

TEST_F(ControlTaskTest, OffDisablesLoopsAndGatesDuty)
{
    EXPECT_CALL(voltageLoop, step(Field(&ctrl::VoltageLoopInputs::enable, false), _));
    EXPECT_CALL(currentLoop, step(Field(&ctrl::CurrentLoopInputs::enable, false), _))
        .WillOnce(SetArgReferee<1>(loopOutput(0.3F)));   // P part of a disabled loop
    EXPECT_CALL(pwm, setDuty(0.0F));

    task.setCommand(command(false));
    task.stepVoltageLoop();
    task.stepCurrentLoop();
    EXPECT_FALSE(task.isOutputEnabled());
}

TEST_F(ControlTaskTest, SwitchOnClearsTripBeforeDriving)
{
    ON_CALL(currentLoop, step(_, _)).WillByDefault(SetArgReferee<1>(loopOutput(0.2F)));
    {
        InSequence sequence;
        EXPECT_CALL(trip, clearTrip());
        EXPECT_CALL(pwm, setDuty(0.2F));
    }
    task.setCommand(command(true));
    task.stepCurrentLoop();
    EXPECT_TRUE(task.isOutputEnabled());
}

TEST_F(ControlTaskTest, SetpointChangeWhileOnDoesNotTouchTrip)
{
    task.setCommand(command(true));

    EXPECT_CALL(trip, clearTrip()).Times(0);
    EXPECT_CALL(trip, forceTrip()).Times(0);
    app::ControlCommand c = command(true);
    c.voltageSetpoint = 5.0F;
    task.setCommand(c);

    EXPECT_CALL(voltageLoop, step(Field(&ctrl::VoltageLoopInputs::reference, 5.0F), _));
    task.stepVoltageLoop();
}

TEST_F(ControlTaskTest, SwitchOffForcesTrip)
{
    task.setCommand(command(true));

    EXPECT_CALL(trip, forceTrip());
    EXPECT_CALL(pwm, setDuty(0.0F));
    task.setCommand(command(false));
    task.stepCurrentLoop();
}

TEST_F(ControlTaskTest, HardwareTripLatchesOffUntilOffOnCommand)
{
    ON_CALL(currentLoop, step(_, _)).WillByDefault(SetArgReferee<1>(loopOutput(0.2F)));
    task.setCommand(command(true));
    ON_CALL(trip, isTripped()).WillByDefault(Return(true));

    EXPECT_CALL(pwm, setDuty(0.0F)).Times(2);
    EXPECT_CALL(currentLoop, step(Field(&ctrl::CurrentLoopInputs::enable, false), _)).Times(2);
    task.stepCurrentLoop();
    task.setCommand(command(true));     // same command: stays off
    task.stepCurrentLoop();
    EXPECT_FALSE(task.isOutputEnabled());

    ON_CALL(trip, isTripped()).WillByDefault(Return(false));
    task.setCommand(command(false));
    EXPECT_CALL(trip, clearTrip());
    EXPECT_CALL(currentLoop, step(Field(&ctrl::CurrentLoopInputs::enable, true), _))
        .WillOnce(SetArgReferee<1>(loopOutput(0.2F)));
    EXPECT_CALL(pwm, setDuty(0.2F));
    task.setCommand(command(true));
    task.stepCurrentLoop();
    EXPECT_TRUE(task.isOutputEnabled());
}
