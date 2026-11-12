#ifndef HAL_IANALOGINPUT_HPP
#define HAL_IANALOGINPUT_HPP

#include <stdint.h>

namespace hal {

// One analog measurement: the latest raw ADC result of a converted channel.
class IAnalogInput {
public:
    // Latest conversion result, right-aligned (0..4095 at 12 bit).
    virtual uint16_t readRaw() const = 0;

protected:
    ~IAnalogInput() {}
};

} // namespace hal

#endif
