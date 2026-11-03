#ifndef APP_BLINKER_HPP
#define APP_BLINKER_HPP

#include <stdint.h>

#include "hal/IDigitalOutput.hpp"

namespace app {

// Toggles an output every ticksPerToggle calls of tick(). Called from a
// periodic timer ISR as a sign of life of the core.
class Blinker {
public:
    Blinker(hal::IDigitalOutput& output, uint16_t ticksPerToggle);

    void tick();

private:
    hal::IDigitalOutput& output_;
    const uint16_t ticksPerToggle_;
    uint16_t count_;
};

} // namespace app

#endif
