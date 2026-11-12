// Checks that the mocks of the ctrl interfaces match their interfaces. The
// control task that uses them comes in Phase 7.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockCurrentLoop.hpp"
#include "MockVoltageLoop.hpp"

using ::testing::_;
using ::testing::Invoke;

TEST(CtrlMocksTest, CurrentLoopMock)
{
    MockCurrentLoop mock;
    ctrl::ICurrentLoop& loop = mock;

    EXPECT_CALL(mock, reset());
    EXPECT_CALL(mock, step(_, _))
        .WillOnce(Invoke([](const ctrl::CurrentLoopInputs& in, ctrl::ControlLoopOutputs& out) {
            out.output = 0.5F;
            out.atSetpoint = true;
            out.rampedSetpoint = in.reference;
        }));

    loop.reset();
    ctrl::CurrentLoopInputs in = {};
    in.reference = 1.5F;
    ctrl::ControlLoopOutputs out = {};
    loop.step(in, out);

    EXPECT_FLOAT_EQ(out.output, 0.5F);
    EXPECT_TRUE(out.atSetpoint);
    EXPECT_FLOAT_EQ(out.rampedSetpoint, 1.5F);
}

TEST(CtrlMocksTest, VoltageLoopMock)
{
    MockVoltageLoop mock;
    ctrl::IVoltageLoop& loop = mock;

    EXPECT_CALL(mock, reset());
    EXPECT_CALL(mock, step(_, _))
        .WillOnce(Invoke([](const ctrl::VoltageLoopInputs& in, ctrl::ControlLoopOutputs& out) {
            out.output = in.openLoopSetpoint;
            out.atSetpoint = false;
            out.rampedSetpoint = in.reference;
        }));

    loop.reset();
    ctrl::VoltageLoopInputs in = {};
    in.reference = 12.0F;
    in.mode = ctrl::VoltageLoopInputs::Mode::kOpenLoop;
    in.openLoopSetpoint = 3.0F;
    ctrl::ControlLoopOutputs out = {};
    loop.step(in, out);

    EXPECT_FLOAT_EQ(out.output, 3.0F);
    EXPECT_FALSE(out.atSetpoint);
    EXPECT_FLOAT_EQ(out.rampedSetpoint, 12.0F);
}
