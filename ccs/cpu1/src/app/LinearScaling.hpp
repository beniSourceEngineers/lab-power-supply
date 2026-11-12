#ifndef APP_LINEARSCALING_HPP
#define APP_LINEARSCALING_HPP

#include <stdint.h>

namespace app {

// Raw ADC result to a physical value: value = raw * gain + offset.
struct LinearScaling {
    float gain;
    float offset;

    float apply(uint16_t raw) const
    {
        return static_cast<float>(raw) * gain + offset;
    }
};

} // namespace app

#endif
