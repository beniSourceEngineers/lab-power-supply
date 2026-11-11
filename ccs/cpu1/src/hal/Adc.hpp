#ifndef HAL_ADC_HPP
#define HAL_ADC_HPP

#include <stdint.h>

#include "driverlib.h"

namespace hal {

// One ADC module (ADCA..ADCD) in 12-bit single-ended mode: power-up, SOC set-up
// and the end-of-conversion interrupt. The results are read through AdcInput.
// Only the composition root uses this class, so it has no interface.
class Adc {
public:
    explicit Adc(uint32_t base);

    // ADCCLK = SYSCLK / 4 (50 MHz), 12 bit single-ended, interrupt pulse at the
    // end of conversion, converter powered up. The caller must wait 1 ms
    // before the first conversion (DEVICE_DELAY_US(1000)).
    void powerUp();

    // sampleWindow in SYSCLK cycles; at least 75 ns (15 cycles) at 12 bit.
    void configureSoc(ADC_SOCNumber soc, ADC_Trigger trigger, ADC_Channel channel,
                      uint32_t sampleWindow);

    // Raise interrupt intNumber at the end of conversion of soc. Continuous
    // mode: a new pulse is generated even if the flag is still set, and an
    // overflow is reported by acknowledgeInterrupt().
    void enableEndOfConversionInterrupt(ADC_IntNumber intNumber, ADC_SOCNumber soc);

    // Clears the interrupt flag; call at the end of the ISR. Returns true if an
    // interrupt was missed (overflow), which is then cleared as well.
    bool acknowledgeInterrupt(ADC_IntNumber intNumber);

private:
    const uint32_t base_;
};

} // namespace hal

#endif
