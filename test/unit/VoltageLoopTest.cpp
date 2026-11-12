// ctrl::VoltageLoop against the generated Voltage_Control code. The expected
// values follow from the model parameters in control/generated/Voltage_Control/:
// Ts = 1e-4 s, Kp = 0.01, Ki = 1, output limited to 0..15, at-setpoint band
// 0.5 V, slew rate 1e5 V/s in the first step after initialisation.
#include <gtest/gtest.h>

#include "ctrl/VoltageLoop.hpp"

namespace {

ctrl::VoltageLoopInputs makeInputs(float reference, float measurement)
{
    ctrl::VoltageLoopInputs in = {};
    in.reference = reference;
    in.measurement = measurement;
    in.enable = true;
    in.slewRateRise = 1000.0F;    // 0.1 V per step
    in.slewRateFall = 2000.0F;    // 0.2 V per step
    in.mode = ctrl::VoltageLoopInputs::Mode::kClosedLoop;
    in.openLoopSetpoint = 0.0F;
    return in;
}

} // namespace

class VoltageLoopTest : public ::testing::Test {
protected:
    ctrl::ControlLoopOutputs step(const ctrl::VoltageLoopInputs& in)
    {
        ctrl::ControlLoopOutputs out = {};
        loop.step(in, out);
        return out;
    }

    ctrl::VoltageLoop loop;
};

TEST_F(VoltageLoopTest, MapsInputsAndOutputsOfFirstStep)
{
    const ctrl::ControlLoopOutputs out = step(makeInputs(2.0F, 1.0F));

    EXPECT_FLOAT_EQ(out.rampedSetpoint, 2.0F);
    EXPECT_FLOAT_EQ(out.output, 0.01F * 1.0F);
    EXPECT_FALSE(out.atSetpoint);
}

TEST_F(VoltageLoopTest, IntegratesError)
{
    step(makeInputs(2.0F, 1.0F));
    const ctrl::ControlLoopOutputs out = step(makeInputs(2.0F, 1.0F));

    EXPECT_FLOAT_EQ(out.output, 0.01F * 1.0F + 1.0F * 1.0F * 1e-4F);
}

TEST_F(VoltageLoopTest, AtSetpointWithinBand)
{
    EXPECT_TRUE(step(makeInputs(2.0F, 1.7F)).atSetpoint);
}

TEST_F(VoltageLoopTest, LimitsRisingAndFallingSetpoint)
{
    step(makeInputs(5.0F, 5.0F));

    EXPECT_NEAR(step(makeInputs(10.0F, 5.0F)).rampedSetpoint, 5.1F, 1e-5F);
    EXPECT_NEAR(step(makeInputs(0.0F, 5.0F)).rampedSetpoint, 4.9F, 1e-5F);
}

TEST_F(VoltageLoopTest, SaturatesOutput)
{
    EXPECT_FLOAT_EQ(step(makeInputs(0.0F, 5.0F)).output, 0.0F);

    ctrl::VoltageLoopInputs in = makeInputs(5000.0F, 0.0F);
    in.slewRateRise = 1e8F;       // ramp reaches the reference at once
    EXPECT_FLOAT_EQ(step(in).output, 15.0F);
}

TEST_F(VoltageLoopTest, OpenLoopPassesSetpointThrough)
{
    ctrl::VoltageLoopInputs in = makeInputs(12.0F, 0.0F);
    in.mode = ctrl::VoltageLoopInputs::Mode::kOpenLoop;
    in.openLoopSetpoint = 3.5F;

    EXPECT_FLOAT_EQ(step(in).output, 3.5F);
}

TEST_F(VoltageLoopTest, DisableRestartsRampAndResetsIntegrator)
{
    step(makeInputs(2.0F, 1.0F));
    step(makeInputs(2.0F, 1.0F));

    ctrl::VoltageLoopInputs in = makeInputs(12.0F, 0.0F);
    in.enable = false;
    const ctrl::ControlLoopOutputs out = step(in);

    // Ramp restarts from 0 (one step of 0.1 V), integrator is 0: P part only.
    EXPECT_NEAR(out.rampedSetpoint, 0.1F, 1e-6F);
    EXPECT_NEAR(out.output, 0.01F * 0.1F, 1e-7F);
}

TEST_F(VoltageLoopTest, ResetRestartsLikeANewObject)
{
    step(makeInputs(2.0F, 1.0F));
    step(makeInputs(4.0F, 1.0F));

    loop.reset();

    ctrl::VoltageLoop fresh;
    ctrl::ControlLoopOutputs expected = {};
    fresh.step(makeInputs(2.0F, 1.0F), expected);
    const ctrl::ControlLoopOutputs out = step(makeInputs(2.0F, 1.0F));

    EXPECT_FLOAT_EQ(out.output, expected.output);
    EXPECT_FLOAT_EQ(out.rampedSetpoint, expected.rampedSetpoint);
    EXPECT_EQ(out.atSetpoint, expected.atSetpoint);
}
