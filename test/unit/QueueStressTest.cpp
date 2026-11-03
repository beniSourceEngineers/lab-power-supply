// Producer and consumer in two threads, as a stand-in for the two cores.
#include <thread>

#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "ipc/Queue.hpp"

namespace {

const uint32_t kElements = 2000000UL;

struct Element {
    uint32_t value;
    uint32_t inverted;
};

typedef ipc::Queue<Element, 16U, 2U> StressQueue;

} // namespace

TEST(QueueStressTest, EveryElementArrivesOnceAndInOrder)
{
    fake::resetIpc();
    static ipc::QueueProducerStorage<Element, 16U> producerStorage;
    static ipc::QueueConsumerStorage consumerStorage;
    StressQueue producer(producerStorage, consumerStorage, IPC_CPU1_L_CPU2_R);
    StressQueue consumer(producerStorage, consumerStorage, IPC_CPU2_L_CPU1_R);
    producer.initProducer();
    consumer.initConsumer();

    std::thread producerThread([&]() {
        for (uint32_t i = 0U; i < kElements; ) {
            Element e;
            e.value = i;
            e.inverted = ~i;
            if (producer.push(e)) {
                ++i;
            }
        }
    });

    uint32_t expected = 0U;
    uint32_t wrong = 0U;
    while (expected < kElements && wrong == 0U) {
        Element e;
        if (consumer.pop(e)) {
            wrong += (e.value == expected && e.inverted == ~expected) ? 0U : 1U;
            ++expected;
        }
    }
    // On a failure the producer may be stuck on a full queue: keep draining.
    Element drain;
    while (expected < kElements) {
        if (consumer.pop(drain)) {
            ++expected;
        }
    }
    producerThread.join();

    EXPECT_EQ(wrong, 0U);
    EXPECT_EQ(expected, kElements);
    EXPECT_FALSE(consumer.pop(drain));
}
