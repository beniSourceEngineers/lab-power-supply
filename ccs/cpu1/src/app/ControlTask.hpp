#ifndef APP_CONTROLTASK_HPP
#define APP_CONTROLTASK_HPP

#include <stdint.h>

#include "app/LinearScaling.hpp"
#include "app/Measurements.hpp"
#include "ctrl/ControlLoopTypes.hpp"
#include "ctrl/ICurrentLoop.hpp"
#include "ctrl/IVoltageLoop.hpp"
#include "hal/IAnalogInput.hpp"
#include "hal/IPwm.hpp"
#include "hal/ITripZone.hpp"

namespace app {

// Command from the background loop (later: state machine and CPU2).
struct ControlCommand {
    bool outputEnable;
    // kClosedLoop: constant voltage, the voltage loop sets the current reference.
    // kOpenLoop: constant current, the current reference is currentSetpoint.
    ctrl::VoltageLoopInputs::Mode::Type mode;
    float voltageSetpoint;      // V
    float currentSetpoint;      // A, used in kOpenLoop
};

// Slew rate limits of the setpoint ramps (inputs of the generated loops).
struct ControlParameters {
    float voltageSlewRateRise;  // V/s
    float voltageSlewRateFall;  // V/s
    float currentSlewRateRise;  // A/s
    float currentSlewRateFall;  // A/s
};

// Cascaded control of the power stage with the generated control loops
// (AGENTS.md §3.3). Two ISRs at the control rate (10 kHz):
//  - stepVoltageLoop(): voltage loop; its output is the current reference,
//  - stepCurrentLoop(): current loop; its output is the PWM duty cycle.
// Both ISRs are in PIE group 1 without nesting, so they never interrupt each
// other and the current reference needs no protection. setCommand() runs in
// the background loop.
class ControlTask {
public:
    ControlTask(const hal::IAnalogInput& voltageInput, const hal::IAnalogInput& currentInput,
                const LinearScaling& voltageScaling, const LinearScaling& currentScaling,
                const ControlParameters& parameters,
                ctrl::IVoltageLoop& voltageLoop, ctrl::ICurrentLoop& currentLoop,
                hal::IPwm& pwm, hal::ITripZone& tripZone);

    // Switching on clears the trip and enables both loops (their ramps start
    // from 0); switching off forces the trip and disables them. After a trip
    // the output stays off until it is switched off and on again.
    void setCommand(const ControlCommand& command);

    // Voltage ISR: measure the voltage, step the voltage loop.
    void stepVoltageLoop();

    // Current ISR: measure the current, step the current loop, set the duty.
    void stepCurrentLoop();

    bool isOutputEnabled() const { return outputEnabled_; }
    const Measurements& measurements() const { return measurements_; }
    const ctrl::ControlLoopOutputs& voltageLoopOutputs() const { return voltageLoopOutputs_; }
    const ctrl::ControlLoopOutputs& currentLoopOutputs() const { return currentLoopOutputs_; }
    uint32_t voltageStepCount() const { return voltageStepCount_; }
    uint32_t currentStepCount() const { return currentStepCount_; }

private:
    const hal::IAnalogInput& voltageInput_;
    const hal::IAnalogInput& currentInput_;
    const LinearScaling voltageScaling_;
    const LinearScaling currentScaling_;
    const ControlParameters parameters_;
    ctrl::IVoltageLoop& voltageLoop_;
    ctrl::ICurrentLoop& currentLoop_;
    hal::IPwm& pwm_;
    hal::ITripZone& tripZone_;

    bool commandedEnable_;              // last command, background only
    // Shared with the ISRs; each 32 bit or less, so each access is atomic.
    volatile bool outputEnabled_;
    volatile ctrl::VoltageLoopInputs::Mode::Type mode_;
    volatile float voltageSetpoint_;
    volatile float currentSetpoint_;

    float currentReference_;            // voltage ISR -> current ISR
    Measurements measurements_;
    ctrl::ControlLoopOutputs voltageLoopOutputs_;
    ctrl::ControlLoopOutputs currentLoopOutputs_;
    uint32_t voltageStepCount_;
    uint32_t currentStepCount_;
};

} // namespace app

#endif
