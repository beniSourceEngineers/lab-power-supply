#ifndef TEST_MOCKDIGITALOUTPUT_HPP
#define TEST_MOCKDIGITALOUTPUT_HPP

#include <gmock/gmock.h>

#include "hal/IDigitalOutput.hpp"

class MockDigitalOutput : public hal::IDigitalOutput {
public:
    MOCK_METHOD(void, write, (bool high), (override));
    MOCK_METHOD(void, toggle, (), (override));
};

#endif
