#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "ipc/Queue.hpp"

namespace {

const uint16_t kSize = 4U;
const uint16_t kFlag = 2U;
const uint32_t kFlagMask = 1UL << kFlag;

typedef ipc::Queue<uint32_t, kSize, kFlag> TestQueue;

} // namespace

class QueueTest : public ::testing::Test {
protected:
    QueueTest()
        : producer(producerStorage, consumerStorage, IPC_CPU1_L_CPU2_R)
        , consumer(producerStorage, consumerStorage, IPC_CPU2_L_CPU1_R)
    {
    }

    void SetUp() override
    {
        fake::resetIpc();
        // Contents from before a reset, which the init functions have to clear.
        producerStorage.head = 3U;
        consumerStorage.tail = 1U;
        producer.initProducer();
        consumer.initConsumer();
    }

    void setIndexes(uint16_t index)
    {
        producerStorage.head = index;
        consumerStorage.tail = index;
    }

    ipc::QueueProducerStorage<uint32_t, kSize> producerStorage;
    ipc::QueueConsumerStorage consumerStorage;
    TestQueue producer;
    TestQueue consumer;
};

TEST_F(QueueTest, EmptyAfterInit)
{
    uint32_t out = 99U;
    EXPECT_FALSE(consumer.pop(out));
    EXPECT_EQ(out, 99U);
}

TEST_F(QueueTest, ElementsArriveInOrder)
{
    EXPECT_TRUE(producer.push(1U));
    EXPECT_TRUE(producer.push(2U));

    uint32_t out = 0U;
    EXPECT_TRUE(consumer.pop(out));
    EXPECT_EQ(out, 1U);
    EXPECT_TRUE(consumer.pop(out));
    EXPECT_EQ(out, 2U);
    EXPECT_FALSE(consumer.pop(out));
}

TEST_F(QueueTest, AllSlotsAreUsableAndFullQueueRejectsPush)
{
    for (uint32_t i = 0U; i < kSize; ++i) {
        EXPECT_TRUE(producer.push(i));
    }
    EXPECT_FALSE(producer.push(100U));

    uint32_t out = 0U;
    for (uint32_t i = 0U; i < kSize; ++i) {
        EXPECT_TRUE(consumer.pop(out));
        EXPECT_EQ(out, i);   // the rejected element did not overwrite anything
    }
}

TEST_F(QueueTest, PopMakesRoomForPush)
{
    for (uint32_t i = 0U; i < kSize; ++i) {
        EXPECT_TRUE(producer.push(i));
    }
    uint32_t out = 0U;
    EXPECT_TRUE(consumer.pop(out));
    EXPECT_TRUE(producer.push(100U));
    EXPECT_FALSE(producer.push(101U));
}

TEST_F(QueueTest, SlotsWrapAround)
{
    uint32_t out = 0U;
    for (uint32_t i = 0U; i < 3U * kSize + 1U; ++i) {
        EXPECT_TRUE(producer.push(i));
        EXPECT_TRUE(consumer.pop(out));
        EXPECT_EQ(out, i);
    }
}

TEST_F(QueueTest, IndexWrapAt16BitKeepsOrderAndFillLevel)
{
    setIndexes(0xFFFEU);
    for (uint32_t i = 0U; i < kSize; ++i) {
        EXPECT_TRUE(producer.push(i));
    }
    EXPECT_EQ(producerStorage.head, 2U);
    EXPECT_FALSE(producer.push(100U));   // full, although head < tail numerically

    uint32_t out = 0U;
    for (uint32_t i = 0U; i < kSize; ++i) {
        EXPECT_TRUE(consumer.pop(out));
        EXPECT_EQ(out, i);
    }
    EXPECT_FALSE(consumer.pop(out));
}

TEST_F(QueueTest, PushSetsFlagAndPopAcknowledgesIt)
{
    EXPECT_TRUE(producer.push(1U));
    EXPECT_TRUE(IPC_isFlagBusyRtoL(IPC_CPU2_L_CPU1_R, kFlagMask));

    uint32_t out = 0U;
    EXPECT_TRUE(consumer.pop(out));
    EXPECT_FALSE(IPC_isFlagBusyRtoL(IPC_CPU2_L_CPU1_R, kFlagMask));
}

TEST_F(QueueTest, RejectedPushDoesNotSetFlag)
{
    for (uint32_t i = 0U; i < kSize; ++i) {
        EXPECT_TRUE(producer.push(i));
    }
    IPC_ackFlagRtoL(IPC_CPU2_L_CPU1_R, kFlagMask);

    EXPECT_FALSE(producer.push(100U));
    EXPECT_FALSE(IPC_isFlagBusyRtoL(IPC_CPU2_L_CPU1_R, kFlagMask));
}

TEST_F(QueueTest, ProducerWritesOnlyHeadAndConsumerOnlyTail)
{
    // Each core may only write its own message RAM.
    EXPECT_TRUE(producer.push(1U));
    EXPECT_EQ(consumerStorage.tail, 0U);

    uint32_t out = 0U;
    EXPECT_TRUE(consumer.pop(out));
    EXPECT_EQ(producerStorage.head, 1U);
    EXPECT_EQ(consumerStorage.tail, 1U);
}
