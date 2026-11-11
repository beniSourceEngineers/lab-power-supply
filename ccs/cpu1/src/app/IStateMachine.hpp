#ifndef APP_ISTATEMACHINE_HPP
#define APP_ISTATEMACHINE_HPP

#include "app/StateMachineTypes.hpp"

namespace app {

// State machine of the power supply (AGENTS.md §3.7). Two variants implement it:
//  - B1: wrapper around the code generated from control/models/StateMachine.slx
//  - B2: handwritten C++03 FSM (Phase 6a)
// The composition root selects one with the build configuration.
class IStateMachine {
public:
    // Back to the initial state (Off). Calls the model's initialize for B1.
    virtual void reset() = 0;

    // One step at the control rate: events in, state and output commands out.
    virtual void step(const StateMachineInputs& in, StateMachineOutputs& out) = 0;

protected:
    ~IStateMachine() {}
};

} // namespace app

#endif
