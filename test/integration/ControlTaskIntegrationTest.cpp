// app::ControlTask with the real control loop wrappers and the generated
// control code; the hardware is mocked at the HAL interfaces. A simple plant
// closes the loop: duty -> output current, current -> output voltage.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockAnalogInput.hpp"
#include "MockPwm.hpp"
#include "MockTripZone.hpp"
#include "app/ControlTask.hpp"
#include "ctrl/CurrentLoop.hpp"
#include "ctrl/VoltageLoop.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

typedef ctrl::VoltageLoopInputs::Mode Mode;

// Scaling 1 count = 1 mV / 1 mA, so the plant works in physical units.
const app::LinearScaling kScaling = { 0.001F, 0.0F };
const app::ControlParameters kParameters = { 1000.0F, 1000.0F, 1000.0F, 1000.0F };

// Plant: the current follows the duty with a first-order lag (10 A at duty 1),
// the output voltage is the current into a 4 ohm load.
struct Plant {
    Plant() : duty(0.0F), current(0.0F) {}
    void advance() { current += 0.05F * (10.0F * duty - current); }
    uint16_t currentRaw() const { return static_cast<uint16_t>(current * 1000.0F); }
    uint16_t voltageRaw() const { return static_cast<uint16_t>(current * 4.0F * 1000.0F); }
    float duty;
    float current;
};

} // namespace

class ControlTaskIntegrationTest : public ::testing::Test {
protected:
    ControlTaskIntegrationTest()
        : task(voltageInput, currentInput, kScaling, kScaling, kParameters,
               voltageLoop, currentLoop, pwm, trip)
    {
        ON_CALL(voltageInput, readRaw()).WillByDefault(Invoke(&plant, &Plant::voltageRaw));
        ON_CALL(currentInput, readRaw()).WillByDefault(Invoke(&plant, &Plant::currentRaw));
        ON_CALL(pwm, setDuty(_)).WillByDefault(Invoke(this, &ControlTaskIntegrationTest::setDuty));
        ON_CALL(trip, isTripped()).WillByDefault(Return(false));
    }

    void setDuty(float duty) { plant.duty = duty; }

    // One control period: current ISR, then voltage ISR (ADC A before ADC B).
    void run(uint32_t periods)
    {
        for (uint32_t i = 0U; i < periods; ++i) {
            task.stepCurrentLoop();
            task.stepVoltageLoop();
            plant.advance();
        }
    }

    static app::ControlCommand command(Mode::Type mode, float voltage, float current)
    {
        app::ControlCommand c;
        c.outputEnable = true;
        c.mode = mode;
        c.voltageSetpoint = voltage;
        c.currentSetpoint = current;
        return c;
    }

    Plant plant;
    NiceMock<MockAnalogInput> voltageInput;
    NiceMock<MockAnalogInput> currentInput;
    NiceMock<MockPwm> pwm;
    NiceMock<MockTripZone> trip;
    ctrl::VoltageLoop voltageLoop;
    ctrl::CurrentLoop currentLoop;
    app::ControlTask task;
};

TEST_F(ControlTaskIntegrationTest, OffKeepsDutyAtZero)
{
    run(1000U);
    EXPECT_FLOAT_EQ(plant.duty, 0.0F);
}

TEST_F(ControlTaskIntegrationTest, ConstantVoltageReachesSetpoint)
{
    task.setCommand(command(Mode::kClosedLoop, 4.0F, 0.0F));
    run(50000U);    // 5 s

    EXPECT_NEAR(task.measurements().outputVoltage, 4.0F, 0.05F);
    EXPECT_TRUE(task.voltageLoopOutputs().atSetpoint);
    EXPECT_NEAR(task.voltageLoopOutputs().output, 1.0F, 0.05F);   // current reference
}

TEST_F(ControlTaskIntegrationTest, ConstantCurrentReachesSetpoint)
{
    task.setCommand(command(Mode::kOpenLoop, 0.0F, 0.5F));
    run(50000U);

    EXPECT_NEAR(task.measurements().outputCurrent, 0.5F, 0.01F);
    EXPECT_TRUE(task.currentLoopOutputs().atSetpoint);
}
