#ifndef IPC_MAILBOXSTORAGE_HPP
#define IPC_MAILBOXSTORAGE_HPP

#include <stdint.h>

namespace ipc {

// Shared memory of a Mailbox, in the writer's message RAM. POD with the same
// layout on both cores.
template <typename T>
struct MailboxStorage {
    volatile uint32_t seq;   // odd while a write is in progress, 0 = never written
    volatile T payload;
};

} // namespace ipc

#endif
