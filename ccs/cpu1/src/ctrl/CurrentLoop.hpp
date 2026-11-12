#ifndef CTRL_CURRENTLOOP_HPP
#define CTRL_CURRENTLOOP_HPP

#include "ctrl/ICurrentLoop.hpp"

extern "C" {
#include "Current_Control.h"
}

namespace ctrl {

// Wrapper around the generated Current_Control model (reusable code, one instance
// per object). Owns the model data; inputs and outputs only through the structs
// in ControlLoopTypes.hpp, so the loop can move to the CLA later (§3.4).
class CurrentLoop : public ICurrentLoop {
public:
    // Initialises the model, so the object is ready to step.
    CurrentLoop();

    virtual void reset();
    virtual void step(const CurrentLoopInputs& in, ControlLoopOutputs& out);

private:
    CurrentLoop(const CurrentLoop&);              // not copyable: model points to own data
    CurrentLoop& operator=(const CurrentLoop&);

    DW_Current_Control_T dwork_;
    PrevZCX_Current_Control_T prevZc_;
    RT_MODEL_Current_Control_T model_;
    ExtU_Current_Control_T inputs_;
    ExtY_Current_Control_T outputs_;
};

} // namespace ctrl

#endif
