// Instantiates all members of the IPC templates, so that the C++03 build of the
// production code also checks them.
#include <stdint.h>

#include "ipc/Mailbox.hpp"
#include "ipc/Queue.hpp"

namespace {

struct Message {
    uint32_t a;
    uint16_t b;
};

} // namespace

template class ipc::Mailbox<Message, 1U>;
template class ipc::Queue<Message, 4U, 2U>;
