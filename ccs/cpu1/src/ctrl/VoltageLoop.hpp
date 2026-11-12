#ifndef CTRL_VOLTAGELOOP_HPP
#define CTRL_VOLTAGELOOP_HPP

#include "ctrl/IVoltageLoop.hpp"

extern "C" {
#include "Voltage_Control.h"
}

namespace ctrl {

// Wrapper around the generated Voltage_Control model (reusable code, one instance
// per object). Owns the model data; inputs and outputs only through the structs
// in ControlLoopTypes.hpp, so the loop can move to the CLA later (§3.4).
class VoltageLoop : public IVoltageLoop {
public:
    // Initialises the model, so the object is ready to step.
    VoltageLoop();

    virtual void reset();
    virtual void step(const VoltageLoopInputs& in, ControlLoopOutputs& out);

private:
    VoltageLoop(const VoltageLoop&);              // not copyable: model points to own data
    VoltageLoop& operator=(const VoltageLoop&);

    DW_Voltage_Control_T dwork_;
    PrevZCX_Voltage_Control_T prevZc_;
    RT_MODEL_Voltage_Control_T model_;
    ExtU_Voltage_Control_T inputs_;
    ExtY_Voltage_Control_T outputs_;
};

} // namespace ctrl

#endif
