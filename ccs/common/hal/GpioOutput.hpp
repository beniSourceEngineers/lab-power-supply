#ifndef HAL_GPIOOUTPUT_HPP
#define HAL_GPIOOUTPUT_HPP

#include <stdint.h>

#include "hal/IDigitalOutput.hpp"

namespace hal {

// GPIO pin used as output, through driverlib.
// Pad configuration, direction and core ownership are set up by CPU1 at boot;
// this class only drives the pin and works on the core that owns it.
class GpioOutput : public IDigitalOutput {
public:
    explicit GpioOutput(uint32_t pin);

    virtual void write(bool high);
    virtual void toggle();

private:
    const uint32_t pin_;
};

} // namespace hal

#endif
