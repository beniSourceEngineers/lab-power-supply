// ctrl::CurrentLoop against the generated Current_Control code. The expected
// values follow from the model parameters in control/generated/Current_Control/:
// Ts = 1e-4 s, PI in volts with Kp = 0.024 V/A, Ki = 240 V/(A s), output (duty)
// = PI output / max(dcLinkVoltage, 10 V), limited to 0..0.95, at-setpoint band
// 0.5 A, slew rate 1e5 A/s in the first step after initialisation.
// At dcLinkVoltage = 24 V the duty equals the former duty gains Kp = 0.001,
// Ki = 10.
#include <gtest/gtest.h>

#include "ctrl/CurrentLoop.hpp"

namespace {

const float kKp = 0.024F;       // V/A
const float kKi = 240.0F;       // V/(A s)
const float kTs = 1e-4F;        // s
const float kVdc = 24.0F;       // V, DC link voltage of makeInputs()

ctrl::CurrentLoopInputs makeInputs(float reference, float measurement)
{
    ctrl::CurrentLoopInputs in = {};
    in.reference = reference;
    in.measurement = measurement;
    in.enable = true;
    in.slewRateRise = 1000.0F;    // 0.1 A per step
    in.slewRateFall = 500.0F;     // 0.05 A per step
    in.dcLinkVoltage = kVdc;
    return in;
}

} // namespace

class CurrentLoopTest : public ::testing::Test {
protected:
    ctrl::ControlLoopOutputs step(const ctrl::CurrentLoopInputs& in)
    {
        ctrl::ControlLoopOutputs out = {};
        loop.step(in, out);
        return out;
    }

    ctrl::CurrentLoop loop;
};

TEST_F(CurrentLoopTest, MapsInputsAndOutputsOfFirstStep)
{
    // First step: slew rate 1e5 A/s, so the ramp jumps to the reference.
    const ctrl::ControlLoopOutputs out = step(makeInputs(2.0F, 1.0F));

    EXPECT_FLOAT_EQ(out.rampedSetpoint, 2.0F);
    EXPECT_NEAR(out.output, kKp * 1.0F / kVdc, 1e-7F);   // P part, integrator still 0
    EXPECT_NEAR(out.output, 0.001F, 1e-7F);
    EXPECT_FALSE(out.atSetpoint);
}

TEST_F(CurrentLoopTest, IntegratesError)
{
    step(makeInputs(2.0F, 1.0F));
    const ctrl::ControlLoopOutputs out = step(makeInputs(2.0F, 1.0F));

    EXPECT_NEAR(out.output, (kKp + kKi * 1.0F * kTs) / kVdc, 1e-7F);
}

TEST_F(CurrentLoopTest, AtSetpointWithinBand)
{
    EXPECT_TRUE(step(makeInputs(2.0F, 1.6F)).atSetpoint);
}

TEST_F(CurrentLoopTest, LimitsRisingAndFallingSetpoint)
{
    step(makeInputs(1.0F, 1.0F));

    EXPECT_NEAR(step(makeInputs(5.0F, 1.0F)).rampedSetpoint, 1.1F, 1e-6F);
    EXPECT_NEAR(step(makeInputs(5.0F, 1.0F)).rampedSetpoint, 1.2F, 1e-6F);
    EXPECT_NEAR(step(makeInputs(0.0F, 1.0F)).rampedSetpoint, 1.15F, 1e-6F);
}

TEST_F(CurrentLoopTest, SaturatesOutput)
{
    EXPECT_FLOAT_EQ(step(makeInputs(0.0F, 5.0F)).output, 0.0F);

    ctrl::CurrentLoopInputs in = makeInputs(5000.0F, 0.0F);
    in.slewRateRise = 1e8F;       // ramp reaches the reference at once
    EXPECT_FLOAT_EQ(step(in).output, 0.95F);
}

TEST_F(CurrentLoopTest, DisableRestartsRampAndResetsIntegrator)
{
    step(makeInputs(2.0F, 1.0F));
    step(makeInputs(2.0F, 1.0F));

    ctrl::CurrentLoopInputs in = makeInputs(2.0F, 0.0F);
    in.enable = false;
    const ctrl::ControlLoopOutputs out = step(in);

    // Ramp restarts from 0 (one step of 0.1 A), integrator is 0: P part only.
    EXPECT_NEAR(out.rampedSetpoint, 0.1F, 1e-6F);
    EXPECT_NEAR(out.output, kKp * 0.1F / kVdc, 1e-8F);
}

TEST_F(CurrentLoopTest, ResetRestartsLikeANewObject)
{
    step(makeInputs(2.0F, 1.0F));
    step(makeInputs(3.0F, 1.0F));

    loop.reset();

    ctrl::CurrentLoop fresh;
    ctrl::ControlLoopOutputs expected = {};
    fresh.step(makeInputs(2.0F, 1.0F), expected);
    const ctrl::ControlLoopOutputs out = step(makeInputs(2.0F, 1.0F));

    EXPECT_FLOAT_EQ(out.output, expected.output);
    EXPECT_FLOAT_EQ(out.rampedSetpoint, expected.rampedSetpoint);
    EXPECT_EQ(out.atSetpoint, expected.atSetpoint);
}

TEST_F(CurrentLoopTest, InstancesAreIndependent)
{
    ctrl::CurrentLoop other;
    step(makeInputs(2.0F, 1.0F));
    step(makeInputs(2.0F, 1.0F));

    ctrl::ControlLoopOutputs out = {};
    other.step(makeInputs(2.0F, 1.0F), out);

    EXPECT_NEAR(out.output, kKp / kVdc, 1e-7F);
}

TEST_F(CurrentLoopTest, FeedforwardDividesByDcLinkVoltage)
{
    ctrl::CurrentLoopInputs in = makeInputs(2.0F, 1.0F);
    in.dcLinkVoltage = 400.0F;

    EXPECT_NEAR(step(in).output, kKp * 1.0F / 400.0F, 1e-8F);
}

TEST_F(CurrentLoopTest, DutyFollowsDcLinkRippleAtConstantPiOutput)
{
    // Same error, DC link 5 % low: the duty rises by 1/0.95 so that
    // duty * dcLinkVoltage (the voltage at the switch node) stays the same.
    ctrl::CurrentLoop other;
    ctrl::CurrentLoopInputs low = makeInputs(2.0F, 1.0F);
    low.dcLinkVoltage = 0.95F * kVdc;
    ctrl::ControlLoopOutputs outLow = {};
    other.step(low, outLow);

    const ctrl::ControlLoopOutputs outNominal = step(makeInputs(2.0F, 1.0F));

    EXPECT_NEAR(outLow.output * low.dcLinkVoltage, outNominal.output * kVdc, 1e-6F);
}

TEST_F(CurrentLoopTest, GuardsLowDcLinkVoltage)
{
    // Below 10 V (including 0 V) the loop divides by 10 V.
    ctrl::CurrentLoopInputs in = makeInputs(2.0F, 1.0F);
    in.dcLinkVoltage = 0.0F;

    EXPECT_NEAR(step(in).output, kKp * 1.0F / 10.0F, 1e-7F);
}

TEST_F(CurrentLoopTest, SaturatesDutyAtAnyDcLinkVoltage)
{
    // The first step ramps by 10 A only (initial slew rate); the error then is
    // so large that the PI output exceeds 0.95 * 400 V.
    ctrl::CurrentLoopInputs in = makeInputs(1e6F, 0.0F);
    in.slewRateRise = 1e12F;
    in.dcLinkVoltage = 400.0F;
    step(in);
    EXPECT_FLOAT_EQ(step(in).output, 0.95F);

    in.dcLinkVoltage = 0.0F;
    EXPECT_FLOAT_EQ(step(in).output, 0.95F);
}
