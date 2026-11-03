#include "app/Blinker.hpp"

namespace app {

Blinker::Blinker(hal::IDigitalOutput& output, uint16_t ticksPerToggle)
    : output_(output)
    , ticksPerToggle_(ticksPerToggle)
    , count_(0U)
{
}

void Blinker::tick()
{
    ++count_;
    if (count_ >= ticksPerToggle_) {
        count_ = 0U;
        output_.toggle();
    }
}

} // namespace app
