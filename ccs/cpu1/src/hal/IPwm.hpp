#ifndef HAL_IPWM_HPP
#define HAL_IPWM_HPP

namespace hal {

// PWM output that drives a switching stage with a duty cycle.
class IPwm {
public:
    // Duty cycle of the high-side output, 0.0 .. 1.0. Values outside are clamped.
    // Takes effect at the next shadow load (counter zero or period).
    virtual void setDuty(float duty) = 0;

protected:
    ~IPwm() {}
};

} // namespace hal

#endif
