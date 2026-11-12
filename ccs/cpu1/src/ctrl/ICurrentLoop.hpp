#ifndef CTRL_ICURRENTLOOP_HPP
#define CTRL_ICURRENTLOOP_HPP

#include "ctrl/ControlLoopTypes.hpp"

namespace ctrl {

// Current control loop (AGENTS.md §3.3). Implemented by ctrl::CurrentLoop, which
// calls the code generated from the Current_Control model.
class ICurrentLoop {
public:
    // Back to the initial state: calls the model's initialize.
    virtual void reset() = 0;

    // One step at the control rate (10 kHz).
    virtual void step(const CurrentLoopInputs& in, ControlLoopOutputs& out) = 0;

protected:
    ~ICurrentLoop() {}
};

} // namespace ctrl

#endif
