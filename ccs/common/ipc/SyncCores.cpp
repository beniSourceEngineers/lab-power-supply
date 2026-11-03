#include "ipc/SyncCores.hpp"

namespace ipc {

void syncCores(IPC_Type_t link, uint16_t flag)
{
    IPC_sync(link, 1UL << flag);
}

} // namespace ipc
