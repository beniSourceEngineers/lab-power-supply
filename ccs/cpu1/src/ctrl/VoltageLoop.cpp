#include "ctrl/VoltageLoop.hpp"

#include <string.h>

namespace ctrl {

VoltageLoop::VoltageLoop()
{
    memset(&model_, 0, sizeof(model_));
    memset(&inputs_, 0, sizeof(inputs_));
    memset(&outputs_, 0, sizeof(outputs_));
    model_.dwork = &dwork_;
    model_.prevZCSigState = &prevZc_;
    Voltage_Control_initialize(&model_);
}

void VoltageLoop::reset()
{
    Voltage_Control_initialize(&model_);
}

void VoltageLoop::step(const VoltageLoopInputs& in, ControlLoopOutputs& out)
{
    VoltageControl_in& u = inputs_.In1;
    u.reference = in.reference;
    u.measurement = in.measurement;
    u.enable = in.enable ? 1U : 0U;
    u.slewrate[0] = in.slewRateRise;
    u.slewrate[1] = in.slewRateFall;
    u.opmode = static_cast<uint16_T>(in.mode);
    u.openLoopSetpoint = in.openLoopSetpoint;

    Voltage_Control_step(&model_, &inputs_, &outputs_);

    const VoltageControl_out& y = outputs_.Out1;
    out.output = y.ctrlOut;
    out.atSetpoint = (y.isAtSetpoint != 0U);
    out.rampedSetpoint = y.rampedSetpoint;
}

} // namespace ctrl
