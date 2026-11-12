#include "hal/AdcInput.hpp"

namespace hal {

AdcInput::AdcInput(uint32_t resultBase, ADC_SOCNumber soc)
    : resultBase_(resultBase)
    , soc_(soc)
{
}

uint16_t AdcInput::readRaw() const
{
    return ADC_readResult(resultBase_, soc_);
}

} // namespace hal
