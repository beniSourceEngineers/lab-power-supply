#ifndef SHARED_TYPES_IPCFLAGS_HPP
#define SHARED_TYPES_IPCFLAGS_HPP

#include <stdint.h>

namespace shared_types {

// Allocation of the IPC flags, per direction. Flags 0-3 can raise an IPC
// interrupt. The CPU2 boot ROM commands use flags 0 and 31 during boot
// (Device_bootCPU2), so channels do not use them.
const uint16_t kIpcFlagSync = 31U;     // ipc::syncCores

} // namespace shared_types

#endif
