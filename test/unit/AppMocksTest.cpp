// Checks that the mocks of app interfaces without a user yet match their
// interfaces. app::IStateMachine is used by the control task in Phase 7.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockStateMachine.hpp"

using ::testing::_;
using ::testing::Invoke;

TEST(AppMocksTest, StateMachineMock)
{
    MockStateMachine mock;
    app::IStateMachine& stateMachine = mock;

    EXPECT_CALL(mock, reset());
    EXPECT_CALL(mock, step(_, _))
        .WillOnce(Invoke([](const app::StateMachineInputs& in, app::StateMachineOutputs& out) {
            out.state = in.outputEnableRequest ? app::State::kSoftStart : app::State::kOff;
            out.powerStageEnable = in.outputEnableRequest;
            out.controlEnable = in.outputEnableRequest;
        }));

    stateMachine.reset();
    app::StateMachineInputs in = {};
    in.outputEnableRequest = true;
    app::StateMachineOutputs out = {};
    stateMachine.step(in, out);

    EXPECT_EQ(out.state, app::State::kSoftStart);
    EXPECT_TRUE(out.powerStageEnable);
    EXPECT_TRUE(out.controlEnable);
}
