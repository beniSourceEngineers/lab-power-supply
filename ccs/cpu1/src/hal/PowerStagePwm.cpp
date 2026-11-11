#include "hal/PowerStagePwm.hpp"

#include "driverlib.h"

namespace hal {

PowerStagePwm::PowerStagePwm(uint32_t base, uint16_t periodCounts)
    : base_(base)
    , periodCounts_(periodCounts)
{
}

void PowerStagePwm::configureOutputs(uint16_t deadBandCounts)
{
    EPWM_setCounterCompareShadowLoadMode(base_, EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO_PERIOD);
    EPWM_setCounterCompareValue(base_, EPWM_COUNTER_COMPARE_A, 0U);

    EPWM_setActionQualifierAction(base_, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base_, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    // Both edges are delayed from xA; xB = inverted xA (active high complementary).
    EPWM_setRisingEdgeDeadBandDelayInput(base_, EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(base_, EPWM_DB_INPUT_EPWMA);
    EPWM_setDeadBandDelayPolarity(base_, EPWM_DB_RED, EPWM_DB_POLARITY_ACTIVE_HIGH);
    EPWM_setDeadBandDelayPolarity(base_, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setDeadBandCounterClock(base_, EPWM_DB_COUNTER_CLOCK_HALF_CYCLE);
    EPWM_setRisingEdgeDelayCount(base_, deadBandCounts);
    EPWM_setFallingEdgeDelayCount(base_, deadBandCounts);
    EPWM_setDeadBandDelayMode(base_, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base_, EPWM_DB_FED, true);
}

void PowerStagePwm::setDuty(float duty)
{
    if (duty < 0.0F) {
        duty = 0.0F;
    } else if (duty > 1.0F) {
        duty = 1.0F;
    }
    const float counts = duty * static_cast<float>(periodCounts_) + 0.5F;
    EPWM_setCounterCompareValue(base_, EPWM_COUNTER_COMPARE_A, static_cast<uint16_t>(counts));
}

} // namespace hal
