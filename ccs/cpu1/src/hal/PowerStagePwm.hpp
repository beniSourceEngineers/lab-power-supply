#ifndef HAL_POWERSTAGEPWM_HPP
#define HAL_POWERSTAGEPWM_HPP

#include <stdint.h>

#include "hal/IPwm.hpp"

namespace hal {

// Complementary half-bridge outputs of one ePWM module in up-down count mode:
// xA is high while the counter is below CMPA (duty = CMPA / TBPRD), xB is its
// complement, both with dead band. The time base is set up with EpwmTimeBase.
class PowerStagePwm : public IPwm {
public:
    PowerStagePwm(uint32_t base, uint16_t periodCounts);

    // Action qualifier, dead band (active high complementary, rising and falling
    // edge delay of deadBandCounts half TBCLK cycles) and CMPA shadow loading at
    // zero and period. Starts with duty 0.
    void configureOutputs(uint16_t deadBandCounts);

    virtual void setDuty(float duty);

private:
    const uint32_t base_;
    const uint16_t periodCounts_;
};

} // namespace hal

#endif
