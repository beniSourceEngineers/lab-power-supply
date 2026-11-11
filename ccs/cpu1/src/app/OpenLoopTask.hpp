#ifndef APP_OPENLOOPTASK_HPP
#define APP_OPENLOOPTASK_HPP

#include <stdint.h>

#include "app/LinearScaling.hpp"
#include "app/Measurements.hpp"
#include "hal/IAnalogInput.hpp"
#include "hal/IPwm.hpp"
#include "hal/ITripZone.hpp"

namespace app {

// Phase 6 bring-up task: drives the power stage open loop with a commanded duty
// cycle and measures output voltage and current. step() runs in the control ISR
// (10 kHz); setCommand() runs in the background loop. Replaced by the control
// task with the generated control loops in Phase 7.
class OpenLoopTask {
public:
    OpenLoopTask(const hal::IAnalogInput& voltageInput, const hal::IAnalogInput& currentInput,
                 const LinearScaling& voltageScaling, const LinearScaling& currentScaling,
                 hal::IPwm& pwm, hal::ITripZone& tripZone);

    // Output on/off and duty cycle (0..1). Switching on clears the trip,
    // switching off forces it. After a trip the output stays off until it is
    // switched off and on again.
    void setCommand(bool outputEnable, float duty);

    // Control ISR: read and scale the measurements, apply the duty cycle.
    void step();

    bool isOutputEnabled() const { return outputEnabled_; }
    const Measurements& measurements() const { return measurements_; }
    uint32_t stepCount() const { return stepCount_; }

private:
    const hal::IAnalogInput& voltageInput_;
    const hal::IAnalogInput& currentInput_;
    const LinearScaling voltageScaling_;
    const LinearScaling currentScaling_;
    hal::IPwm& pwm_;
    hal::ITripZone& tripZone_;

    bool commandedEnable_;              // last command, background only
    volatile bool outputEnabled_;       // shared with the ISR
    volatile float duty_;               // shared with the ISR (32-bit, atomic)
    Measurements measurements_;
    uint32_t stepCount_;
};

} // namespace app

#endif
