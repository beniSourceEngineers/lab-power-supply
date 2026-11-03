#ifndef IPC_QUEUE_HPP
#define IPC_QUEUE_HPP

#include <stdint.h>

#include "driverlib.h"

#include "ipc/IQueue.hpp"
#include "ipc/QueueStorage.hpp"
#include "ipc/SharedCopy.hpp"
#include "util/StaticAssert.hpp"

namespace ipc {

// Ring buffer with one producer and one consumer, one per core. Both cores
// create a Queue on the same two storage objects and use one side of it. Each
// index has one writer and 16-bit accesses are atomic, so no locks are needed.
// Only one context per core may use its side.
//
// Size must be a power of two, so that the free-running indexes stay continuous
// when they wrap. All Size slots are usable (full: head - tail == Size).
//
// The flag only wakes the consumer, as in Mailbox.
template <typename T, uint16_t Size, uint16_t Flag>
class Queue : public IQueue<T> {
public:
    Queue(QueueProducerStorage<T, Size>& producer, QueueConsumerStorage& consumer, IPC_Type_t link)
        : producer_(producer)
        , consumer_(consumer)
        , link_(link)
    {
    }

    // Producer core only, before syncCores(): message RAM keeps its contents
    // over a reset.
    void initProducer()
    {
        producer_.head = 0U;
        IPC_clearFlagLtoR(link_, kFlagMask);
    }

    // Consumer core only, before syncCores().
    void initConsumer()
    {
        consumer_.tail = 0U;
    }

    virtual bool push(const T& element)
    {
        const uint16_t head = producer_.head;
        const uint16_t used = static_cast<uint16_t>(head - consumer_.tail);
        if (used >= Size) {
            return false;
        }

        copyToShared(producer_.slots[head & kIndexMask], element);
        producer_.head = static_cast<uint16_t>(head + 1U);
        IPC_setFlagLtoR(link_, kFlagMask);
        return true;
    }

    virtual bool pop(T& out)
    {
        IPC_ackFlagRtoL(link_, kFlagMask);

        const uint16_t tail = consumer_.tail;
        if (tail == producer_.head) {
            return false;
        }

        copyFromShared(out, producer_.slots[tail & kIndexMask]);
        consumer_.tail = static_cast<uint16_t>(tail + 1U);
        return true;
    }

private:
    STATIC_ASSERT(Size > 0U);
    STATIC_ASSERT(Size <= 0x8000U);
    STATIC_ASSERT((Size & (Size - 1U)) == 0U);
    STATIC_ASSERT(Flag < 32U);

    static const uint32_t kFlagMask = 1UL << Flag;
    static const uint16_t kIndexMask = Size - 1U;

    QueueProducerStorage<T, Size>& producer_;
    QueueConsumerStorage& consumer_;
    const IPC_Type_t link_;
};

template <typename T, uint16_t Size, uint16_t Flag>
const uint32_t Queue<T, Size, Flag>::kFlagMask;

template <typename T, uint16_t Size, uint16_t Flag>
const uint16_t Queue<T, Size, Flag>::kIndexMask;

} // namespace ipc

#endif
