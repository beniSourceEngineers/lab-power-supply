#ifndef TEST_MOCKVOLTAGELOOP_HPP
#define TEST_MOCKVOLTAGELOOP_HPP

#include <gmock/gmock.h>

#include "ctrl/IVoltageLoop.hpp"

class MockVoltageLoop : public ctrl::IVoltageLoop {
public:
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(void, step, (const ctrl::VoltageLoopInputs& in, ctrl::ControlLoopOutputs& out),
                (override));
};

#endif
