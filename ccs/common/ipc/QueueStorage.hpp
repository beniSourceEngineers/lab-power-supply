#ifndef IPC_QUEUESTORAGE_HPP
#define IPC_QUEUESTORAGE_HPP

#include <stdint.h>

namespace ipc {

// A core can only write its own message RAM, so a queue needs memory in both:
// the producer writes head and slots, the consumer writes tail. Both are POD with
// the same layout on both cores. Indexes run freely and wrap at 2^16.

// In the producer's message RAM.
template <typename T, uint16_t Size>
struct QueueProducerStorage {
    volatile uint16_t head;
    volatile T slots[Size];
};

// In the consumer's message RAM.
struct QueueConsumerStorage {
    volatile uint16_t tail;
};

} // namespace ipc

#endif
