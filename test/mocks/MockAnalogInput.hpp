#ifndef TEST_MOCKANALOGINPUT_HPP
#define TEST_MOCKANALOGINPUT_HPP

#include <gmock/gmock.h>

#include "hal/IAnalogInput.hpp"

class MockAnalogInput : public hal::IAnalogInput {
public:
    MOCK_METHOD(uint16_t, readRaw, (), (const, override));
};

#endif
