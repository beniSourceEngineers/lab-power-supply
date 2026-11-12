#ifndef CTRL_CONTROLLOOPTYPES_HPP
#define CTRL_CONTROLLOOPTYPES_HPP

namespace ctrl {

// Inputs and outputs of the generated control loops (control/generated/),
// in C++ types. The wrappers map them to the model buses field by field, so the
// application does not depend on the generated headers. Sample time 1e-4 s.
//
// Model behaviour to know when using the loops:
//  - In the first step after initialisation the slew rate limiter uses 1e5/s
//    (10 units per step) instead of the slewRate inputs.
//  - enable = false does not force the output to 0: the ramp restarts from 0 and
//    the integrator is reset, but the P part still acts on rampedSetpoint -
//    measurement. Gate the PWM separately (state machine powerStageEnable).

// Inputs of one current loop step (bus CurrentControl_in).
struct CurrentLoopInputs {
    float reference;        // A, setpoint before the slew rate limiter
    float measurement;      // A, measured output current
    bool enable;            // false: integrator reset, ramp restarts from 0
    float slewRateRise;     // A/s, limit for a rising setpoint (slewrate[0])
    float slewRateFall;     // A/s, limit for a falling setpoint (slewrate[1]), sign ignored
};

// Inputs of one voltage loop step (bus VoltageControl_in).
struct VoltageLoopInputs {
    // Operating mode (opmode). The model only checks for open loop (2).
    struct Mode {
        enum Type {
            kClosedLoop = 0,
            kOpenLoop = 2       // output = openLoopSetpoint, controller bypassed
        };
    };

    float reference;        // V, setpoint before the slew rate limiter
    float measurement;      // V, measured output voltage
    bool enable;            // false: integrator reset, ramp restarts from 0
    float slewRateRise;     // V/s, limit for a rising setpoint (slewrate[0])
    float slewRateFall;     // V/s, limit for a falling setpoint (slewrate[1]), sign ignored
    Mode::Type mode;
    float openLoopSetpoint; // output in Mode::kOpenLoop
};

// Outputs of one step of either loop (buses CurrentControl_out, VoltageControl_out).
struct ControlLoopOutputs {
    float output;           // controller output after saturation (ctrlOut)
    bool atSetpoint;        // |rampedSetpoint - measurement| within the model's band
    float rampedSetpoint;   // setpoint after the slew rate limiter
};

} // namespace ctrl

#endif
