#include "hal/Adc.hpp"

namespace hal {

Adc::Adc(uint32_t base)
    : base_(base)
{
}

void Adc::powerUp()
{
    ADC_setPrescaler(base_, ADC_CLK_DIV_4_0);
    ADC_setMode(base_, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    ADC_setInterruptPulseMode(base_, ADC_PULSE_END_OF_CONV);
    ADC_enableConverter(base_);
}

void Adc::configureSoc(ADC_SOCNumber soc, ADC_Trigger trigger, ADC_Channel channel,
                       uint32_t sampleWindow)
{
    ADC_setupSOC(base_, soc, trigger, channel, sampleWindow);
}

void Adc::enableEndOfConversionInterrupt(ADC_IntNumber intNumber, ADC_SOCNumber soc)
{
    ADC_setInterruptSource(base_, intNumber, static_cast<uint16_t>(soc));
    ADC_enableContinuousMode(base_, intNumber);
    ADC_enableInterrupt(base_, intNumber);
    ADC_clearInterruptStatus(base_, intNumber);
}

bool Adc::acknowledgeInterrupt(ADC_IntNumber intNumber)
{
    const bool overflow = ADC_getInterruptOverflowStatus(base_, intNumber);
    if (overflow) {
        ADC_clearInterruptOverflowStatus(base_, intNumber);
    }
    ADC_clearInterruptStatus(base_, intNumber);
    return overflow;
}

} // namespace hal
