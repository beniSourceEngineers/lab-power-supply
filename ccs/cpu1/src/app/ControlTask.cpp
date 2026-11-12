#include "app/ControlTask.hpp"

namespace app {

namespace {

ctrl::ControlLoopOutputs zeroOutputs()
{
    ctrl::ControlLoopOutputs out;
    out.output = 0.0F;
    out.atSetpoint = false;
    out.rampedSetpoint = 0.0F;
    return out;
}

} // namespace

ControlTask::ControlTask(const hal::IAnalogInput& voltageInput,
                         const hal::IAnalogInput& currentInput,
                         const LinearScaling& voltageScaling,
                         const LinearScaling& currentScaling,
                         const ControlParameters& parameters,
                         ctrl::IVoltageLoop& voltageLoop, ctrl::ICurrentLoop& currentLoop,
                         hal::IPwm& pwm, hal::ITripZone& tripZone)
    : voltageInput_(voltageInput)
    , currentInput_(currentInput)
    , voltageScaling_(voltageScaling)
    , currentScaling_(currentScaling)
    , parameters_(parameters)
    , voltageLoop_(voltageLoop)
    , currentLoop_(currentLoop)
    , pwm_(pwm)
    , tripZone_(tripZone)
    , commandedEnable_(false)
    , outputEnabled_(false)
    , mode_(ctrl::VoltageLoopInputs::Mode::kClosedLoop)
    , voltageSetpoint_(0.0F)
    , currentSetpoint_(0.0F)
    , currentReference_(0.0F)
    , measurements_()
    , voltageLoopOutputs_(zeroOutputs())
    , currentLoopOutputs_(zeroOutputs())
    , voltageStepCount_(0U)
    , currentStepCount_(0U)
{
}

void ControlTask::setCommand(const ControlCommand& command)
{
    mode_ = command.mode;
    voltageSetpoint_ = command.voltageSetpoint;
    currentSetpoint_ = command.currentSetpoint;

    if (command.outputEnable == commandedEnable_) {
        return;
    }
    commandedEnable_ = command.outputEnable;
    if (command.outputEnable) {
        // Clear first: the ISR must not see the old trip as a new one.
        tripZone_.clearTrip();
        outputEnabled_ = true;
    } else {
        outputEnabled_ = false;
        tripZone_.forceTrip();
    }
}

void ControlTask::stepVoltageLoop()
{
    measurements_.outputVoltage = voltageScaling_.apply(voltageInput_.readRaw());

    ctrl::VoltageLoopInputs in;
    in.reference = voltageSetpoint_;
    in.measurement = measurements_.outputVoltage;
    in.enable = outputEnabled_;
    in.slewRateRise = parameters_.voltageSlewRateRise;
    in.slewRateFall = parameters_.voltageSlewRateFall;
    in.mode = mode_;
    in.openLoopSetpoint = currentSetpoint_;
    voltageLoop_.step(in, voltageLoopOutputs_);

    currentReference_ = voltageLoopOutputs_.output;
    ++voltageStepCount_;
}

void ControlTask::stepCurrentLoop()
{
    measurements_.outputCurrent = currentScaling_.apply(currentInput_.readRaw());

    if (outputEnabled_ && tripZone_.isTripped()) {
        outputEnabled_ = false;     // latched until the next off/on command
    }
    const bool enabled = outputEnabled_;

    ctrl::CurrentLoopInputs in;
    in.reference = currentReference_;
    in.measurement = measurements_.outputCurrent;
    in.enable = enabled;
    in.slewRateRise = parameters_.currentSlewRateRise;
    in.slewRateFall = parameters_.currentSlewRateFall;
    currentLoop_.step(in, currentLoopOutputs_);

    // A disabled loop still outputs its P part (ControlLoopTypes.hpp): gate it.
    pwm_.setDuty(enabled ? currentLoopOutputs_.output : 0.0F);
    ++currentStepCount_;
}

} // namespace app
