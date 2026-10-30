#include "hal/GpioOutput.hpp"

#include "driverlib.h"

namespace hal {

GpioOutput::GpioOutput(uint32_t pin)
    : pin_(pin)
{
}

void GpioOutput::write(bool high)
{
    GPIO_writePin(pin_, high ? 1U : 0U);
}

void GpioOutput::toggle()
{
    GPIO_togglePin(pin_);
}

} // namespace hal
