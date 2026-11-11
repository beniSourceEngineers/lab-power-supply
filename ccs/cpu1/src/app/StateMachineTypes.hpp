#ifndef APP_STATEMACHINETYPES_HPP
#define APP_STATEMACHINETYPES_HPP

namespace app {

// Operating states of the power supply (AGENTS.md §3.3). The values match the
// state enumeration of the Stateflow model (doc/state_machine.md).
struct State {
    enum Type {
        kOff = 0,
        kSoftStart = 1,
        kConstantVoltage = 2,
        kConstantCurrent = 3,
        kFault = 4
    };
};

// Inputs of one state machine step (control rate, 10 kHz).
struct StateMachineInputs {
    bool outputEnableRequest;   // output on/off command from CPU2
    bool faultResetRequest;     // acknowledge a fault (leave Fault when the cause is gone)
    bool faultActive;           // protection or hardware trip active
    bool voltageAtSetpoint;     // voltage loop: ramped setpoint reached the reference
    bool currentLimitActive;    // the current limit determines the output (CC)
};

// Outputs of one state machine step.
struct StateMachineOutputs {
    State::Type state;
    bool powerStageEnable;      // release the PWM outputs (clear the trip)
    bool controlEnable;         // enable inputs of the voltage and current loop
};

} // namespace app

#endif
