#include "app/OpenLoopTask.hpp"

namespace app {

OpenLoopTask::OpenLoopTask(const hal::IAnalogInput& voltageInput,
                           const hal::IAnalogInput& currentInput,
                           const LinearScaling& voltageScaling,
                           const LinearScaling& currentScaling,
                           hal::IPwm& pwm, hal::ITripZone& tripZone)
    : voltageInput_(voltageInput)
    , currentInput_(currentInput)
    , voltageScaling_(voltageScaling)
    , currentScaling_(currentScaling)
    , pwm_(pwm)
    , tripZone_(tripZone)
    , commandedEnable_(false)
    , outputEnabled_(false)
    , duty_(0.0F)
    , measurements_()
    , stepCount_(0U)
{
}

void OpenLoopTask::setCommand(bool outputEnable, float duty)
{
    duty_ = duty;
    if (outputEnable == commandedEnable_) {
        return;
    }
    commandedEnable_ = outputEnable;
    if (outputEnable) {
        // Clear first: the ISR must not see the old trip as a new one.
        tripZone_.clearTrip();
        outputEnabled_ = true;
    } else {
        outputEnabled_ = false;
        tripZone_.forceTrip();
    }
}

void OpenLoopTask::step()
{
    measurements_.outputVoltage = voltageScaling_.apply(voltageInput_.readRaw());
    measurements_.outputCurrent = currentScaling_.apply(currentInput_.readRaw());

    if (outputEnabled_ && tripZone_.isTripped()) {
        outputEnabled_ = false;     // latched until the next off/on command
    }
    pwm_.setDuty(outputEnabled_ ? duty_ : 0.0F);
    ++stepCount_;
}

} // namespace app
