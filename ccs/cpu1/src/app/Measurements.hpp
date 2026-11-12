#ifndef APP_MEASUREMENTS_HPP
#define APP_MEASUREMENTS_HPP

namespace app {

// Scaled measurements of one control period.
struct Measurements {
    float outputVoltage;    // V
    float outputCurrent;    // A
};

} // namespace app

#endif
