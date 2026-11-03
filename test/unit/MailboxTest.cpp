#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "ipc/Mailbox.hpp"
#include "util/StaticAssert.hpp"

namespace {

struct Message {
    uint32_t value;
    uint16_t other;
    uint16_t spare;
};
STATIC_ASSERT_SIZE_WORDS(Message, 4);

const uint16_t kFlag = 3U;
const uint32_t kFlagMask = 1UL << kFlag;

typedef ipc::Mailbox<Message, kFlag> TestMailbox;

Message makeMessage(uint32_t value)
{
    Message m;
    m.value = value;
    m.other = static_cast<uint16_t>(value + 1U);
    m.spare = 0U;
    return m;
}

} // namespace

class MailboxTest : public ::testing::Test {
protected:
    MailboxTest()
        : writer(storage, IPC_CPU1_L_CPU2_R)
        , reader(storage, IPC_CPU2_L_CPU1_R)
    {
    }

    void SetUp() override
    {
        fake::resetIpc();
        // Contents from before a reset, which initWriter has to invalidate.
        storage.seq = 0x1234567UL;
        writer.initWriter();
    }

    ipc::MailboxStorage<Message> storage;
    TestMailbox writer;
    TestMailbox reader;
};

TEST_F(MailboxTest, NothingToReadBeforeFirstPost)
{
    Message out = makeMessage(99U);
    EXPECT_FALSE(reader.hasNew());
    EXPECT_FALSE(reader.tryRead(out));
    EXPECT_EQ(out.value, 99U);
}

TEST_F(MailboxTest, ReadReturnsPostedMessage)
{
    writer.post(makeMessage(42U));
    EXPECT_TRUE(reader.hasNew());

    Message out = makeMessage(0U);
    EXPECT_TRUE(reader.tryRead(out));
    EXPECT_EQ(out.value, 42U);
    EXPECT_EQ(out.other, 43U);
}

TEST_F(MailboxTest, MessageIsReadOnlyOnce)
{
    writer.post(makeMessage(42U));
    Message out;
    EXPECT_TRUE(reader.tryRead(out));

    EXPECT_FALSE(reader.hasNew());
    EXPECT_FALSE(reader.tryRead(out));
}

TEST_F(MailboxTest, ReaderGetsLatestValueOnly)
{
    writer.post(makeMessage(1U));
    writer.post(makeMessage(2U));

    Message out;
    EXPECT_TRUE(reader.tryRead(out));
    EXPECT_EQ(out.value, 2U);
    EXPECT_FALSE(reader.tryRead(out));
}

TEST_F(MailboxTest, WriteInProgressIsNotRead)
{
    writer.post(makeMessage(1U));
    storage.seq = storage.seq + 1UL;   // the state in the middle of post()

    Message out = makeMessage(99U);
    EXPECT_FALSE(reader.hasNew());
    EXPECT_FALSE(reader.tryRead(out));
    EXPECT_EQ(out.value, 99U);

    storage.seq = storage.seq + 1UL;
    EXPECT_TRUE(reader.tryRead(out));
}

TEST_F(MailboxTest, SequenceCounterWrapIsStillNew)
{
    storage.seq = 0xFFFFFFFEUL;
    Message out;
    EXPECT_TRUE(reader.tryRead(out));

    writer.post(makeMessage(7U));   // seq wraps to 0
    EXPECT_EQ(storage.seq, 0UL);
    EXPECT_TRUE(reader.tryRead(out));
    EXPECT_EQ(out.value, 7U);
}

TEST_F(MailboxTest, PostSetsFlagForTheOtherCore)
{
    EXPECT_FALSE(IPC_isFlagBusyRtoL(IPC_CPU2_L_CPU1_R, kFlagMask));
    writer.post(makeMessage(1U));
    EXPECT_TRUE(IPC_isFlagBusyRtoL(IPC_CPU2_L_CPU1_R, kFlagMask));
}

TEST_F(MailboxTest, ReadAcknowledgesFlagEvenWithoutNewMessage)
{
    // Otherwise a spurious flag would raise the IPC interrupt forever.
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, kFlagMask);
    Message out;
    EXPECT_FALSE(reader.tryRead(out));
    EXPECT_FALSE(IPC_isFlagBusyLtoR(IPC_CPU1_L_CPU2_R, kFlagMask));
}

TEST_F(MailboxTest, HasNewDoesNotConsumeMessage)
{
    writer.post(makeMessage(1U));
    EXPECT_TRUE(reader.hasNew());
    EXPECT_TRUE(reader.hasNew());
}

TEST_F(MailboxTest, InitWriterClearsOldFlag)
{
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, kFlagMask);
    writer.initWriter();
    EXPECT_FALSE(IPC_isFlagBusyLtoR(IPC_CPU1_L_CPU2_R, kFlagMask));
}
