#ifndef TEST_MOCKCURRENTLOOP_HPP
#define TEST_MOCKCURRENTLOOP_HPP

#include <gmock/gmock.h>

#include "ctrl/ICurrentLoop.hpp"

class MockCurrentLoop : public ctrl::ICurrentLoop {
public:
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(void, step, (const ctrl::CurrentLoopInputs& in, ctrl::ControlLoopOutputs& out),
                (override));
};

#endif
