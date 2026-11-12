#include "ctrl/CurrentLoop.hpp"

#include <string.h>

namespace ctrl {

CurrentLoop::CurrentLoop()
{
    memset(&model_, 0, sizeof(model_));
    memset(&inputs_, 0, sizeof(inputs_));
    memset(&outputs_, 0, sizeof(outputs_));
    model_.dwork = &dwork_;
    model_.prevZCSigState = &prevZc_;
    Current_Control_initialize(&model_);
}

void CurrentLoop::reset()
{
    Current_Control_initialize(&model_);
}

void CurrentLoop::step(const CurrentLoopInputs& in, ControlLoopOutputs& out)
{
    CurrentControl_in& u = inputs_.In1;
    u.reference = in.reference;
    u.measurement = in.measurement;
    u.enable = in.enable ? 1U : 0U;
    u.slewrate[0] = in.slewRateRise;
    u.slewrate[1] = in.slewRateFall;

    Current_Control_step(&model_, &inputs_, &outputs_);

    const CurrentControl_out& y = outputs_.Out1;
    out.output = y.ctrlOut;
    out.atSetpoint = (y.isAtSetpoint != 0U);
    out.rampedSetpoint = y.rampedSetpoint;
}

} // namespace ctrl
