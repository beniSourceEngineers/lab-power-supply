#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "MockCommandHandler.hpp"
#include "ipc/CommandLink.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace {
const uint16_t kFlag = 5U;
}

class CommandLinkTest : public ::testing::Test {
protected:
    CommandLinkTest()
        : cpu1(IPC_CPU1_L_CPU2_R, kFlag)
        , cpu2(IPC_CPU2_L_CPU1_R, kFlag)
    {
    }

    void SetUp() override { fake::resetIpc(); }

    ipc::CommandLink cpu1;
    ipc::CommandLink cpu2;
    StrictMock<MockCommandHandler> handler;
};

TEST_F(CommandLinkTest, PollWithoutCommandDoesNotCallHandler)
{
    EXPECT_FALSE(cpu2.poll(handler));
}

TEST_F(CommandLinkTest, CommandReachesHandlerAndResponseComesBack)
{
    EXPECT_CALL(handler, onCommand(1U, 2U, 3U)).WillOnce(Return(44U));

    EXPECT_TRUE(cpu1.send(1U, 2U, 3U));
    EXPECT_TRUE(cpu1.isBusy());

    EXPECT_TRUE(cpu2.poll(handler));
    EXPECT_FALSE(cpu1.isBusy());
    EXPECT_EQ(cpu1.response(), 44U);
}

TEST_F(CommandLinkTest, CommandIsHandledOnce)
{
    EXPECT_CALL(handler, onCommand(_, _, _)).WillOnce(Return(0U));
    EXPECT_TRUE(cpu1.send(1U, 2U, 3U));

    EXPECT_TRUE(cpu2.poll(handler));
    EXPECT_FALSE(cpu2.poll(handler));
}

TEST_F(CommandLinkTest, SendFailsWhileCommandIsInFlight)
{
    EXPECT_CALL(handler, onCommand(1U, 0U, 0U)).WillOnce(Return(0U));

    EXPECT_TRUE(cpu1.send(1U, 0U, 0U));
    EXPECT_FALSE(cpu1.send(2U, 0U, 0U));

    EXPECT_TRUE(cpu2.poll(handler));   // still sees the first command
    EXPECT_TRUE(cpu1.send(3U, 0U, 0U));
}

TEST_F(CommandLinkTest, BothDirectionsAreIndependent)
{
    EXPECT_CALL(handler, onCommand(1U, 0U, 0U)).WillOnce(Return(10U));
    EXPECT_CALL(handler, onCommand(2U, 0U, 0U)).WillOnce(Return(20U));

    EXPECT_TRUE(cpu1.send(1U, 0U, 0U));
    EXPECT_TRUE(cpu2.send(2U, 0U, 0U));

    EXPECT_TRUE(cpu2.poll(handler));
    EXPECT_TRUE(cpu1.poll(handler));
    EXPECT_EQ(cpu1.response(), 10U);
    EXPECT_EQ(cpu2.response(), 20U);
}

TEST_F(CommandLinkTest, InitClearsOldFlag)
{
    EXPECT_TRUE(cpu1.send(1U, 0U, 0U));
    cpu1.init();
    EXPECT_FALSE(cpu1.isBusy());
    EXPECT_FALSE(cpu2.poll(handler));
}
