#ifndef HAL_ADCINPUT_HPP
#define HAL_ADCINPUT_HPP

#include <stdint.h>

#include "driverlib.h"
#include "hal/IAnalogInput.hpp"

namespace hal {

// The result register of one ADC SOC. The SOC itself is set up with Adc.
class AdcInput : public IAnalogInput {
public:
    AdcInput(uint32_t resultBase, ADC_SOCNumber soc);

    virtual uint16_t readRaw() const;

private:
    const uint32_t resultBase_;
    const ADC_SOCNumber soc_;
};

} // namespace hal

#endif
