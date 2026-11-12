#ifndef TEST_MOCKSTATEMACHINE_HPP
#define TEST_MOCKSTATEMACHINE_HPP

#include <gmock/gmock.h>

#include "app/IStateMachine.hpp"

class MockStateMachine : public app::IStateMachine {
public:
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(void, step, (const app::StateMachineInputs& in, app::StateMachineOutputs& out),
                (override));
};

#endif
