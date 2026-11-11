#ifndef HAL_ITRIPZONE_HPP
#define HAL_ITRIPZONE_HPP

namespace hal {

// One-shot trip of a PWM module: while tripped, the outputs are forced to a
// safe state by hardware. A trip latches until it is cleared.
class ITripZone {
public:
    // Trip now (software force), e.g. for Off or Fault.
    virtual void forceTrip() = 0;
    // Release the outputs again. A trip source that is still active trips again.
    virtual void clearTrip() = 0;
    virtual bool isTripped() const = 0;

protected:
    ~ITripZone() {}
};

} // namespace hal

#endif
