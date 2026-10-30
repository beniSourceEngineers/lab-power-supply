#ifndef HAL_IDIGITALOUTPUT_HPP
#define HAL_IDIGITALOUTPUT_HPP

namespace hal {

// A single digital output pin (LED, enable line, ...).
class IDigitalOutput {
public:
    virtual void write(bool high) = 0;
    virtual void toggle() = 0;

protected:
    // Protected, non-virtual: objects are never deleted through the interface
    // (no heap), so no deleting destructor and no operator delete is linked.
    ~IDigitalOutput() {}
};

} // namespace hal

#endif
