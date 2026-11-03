// Checks that the IPC mocks match their interfaces. Application code that uses
// them through the interfaces follows in Phase 6.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockCommandHandler.hpp"
#include "MockCommandLink.hpp"
#include "MockMailboxReader.hpp"
#include "MockMailboxWriter.hpp"
#include "MockQueue.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

TEST(IpcMocksTest, MailboxMocks)
{
    MockMailboxWriter<uint32_t> writerMock;
    MockMailboxReader<uint32_t> readerMock;
    ipc::IMailboxWriter<uint32_t>& writer = writerMock;
    ipc::IMailboxReader<uint32_t>& reader = readerMock;

    EXPECT_CALL(writerMock, post(5U));
    EXPECT_CALL(readerMock, hasNew()).WillOnce(Return(true));
    EXPECT_CALL(readerMock, tryRead(_)).WillOnce(DoAll(SetArgReferee<0>(6U), Return(true)));

    writer.post(5U);
    EXPECT_TRUE(reader.hasNew());
    uint32_t out = 0U;
    EXPECT_TRUE(reader.tryRead(out));
    EXPECT_EQ(out, 6U);
}

TEST(IpcMocksTest, QueueMock)
{
    MockQueue<uint32_t> mock;
    ipc::IQueue<uint32_t>& queue = mock;

    EXPECT_CALL(mock, push(5U)).WillOnce(Return(true));
    EXPECT_CALL(mock, pop(_)).WillOnce(DoAll(SetArgReferee<0>(6U), Return(true)));

    EXPECT_TRUE(queue.push(5U));
    uint32_t out = 0U;
    EXPECT_TRUE(queue.pop(out));
    EXPECT_EQ(out, 6U);
}

TEST(IpcMocksTest, CommandMocks)
{
    MockCommandLink linkMock;
    MockCommandHandler handlerMock;
    ipc::ICommandLink& link = linkMock;
    ipc::ICommandHandler& handler = handlerMock;

    EXPECT_CALL(linkMock, send(1U, 2U, 3U)).WillOnce(Return(true));
    EXPECT_CALL(linkMock, isBusy()).WillOnce(Return(false));
    EXPECT_CALL(linkMock, response()).WillOnce(Return(4U));
    EXPECT_CALL(linkMock, poll(_)).WillOnce(Return(false));
    EXPECT_CALL(handlerMock, onCommand(1U, 2U, 3U)).WillOnce(Return(4U));

    EXPECT_TRUE(link.send(1U, 2U, 3U));
    EXPECT_FALSE(link.isBusy());
    EXPECT_EQ(link.response(), 4U);
    EXPECT_FALSE(link.poll(handler));
    EXPECT_EQ(handler.onCommand(1U, 2U, 3U), 4U);
}
