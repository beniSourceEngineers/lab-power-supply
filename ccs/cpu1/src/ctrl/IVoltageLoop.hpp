#ifndef CTRL_IVOLTAGELOOP_HPP
#define CTRL_IVOLTAGELOOP_HPP

#include "ctrl/ControlLoopTypes.hpp"

namespace ctrl {

// Voltage control loop (AGENTS.md §3.3). Implemented by ctrl::VoltageLoop, which
// calls the code generated from the Voltage_Control model.
class IVoltageLoop {
public:
    // Back to the initial state: calls the model's initialize.
    virtual void reset() = 0;

    // One step at the control rate (10 kHz).
    virtual void step(const VoltageLoopInputs& in, ControlLoopOutputs& out) = 0;

protected:
    ~IVoltageLoop() {}
};

} // namespace ctrl

#endif
