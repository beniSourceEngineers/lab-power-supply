#ifndef IPC_SYNCCORES_HPP
#define IPC_SYNCCORES_HPP

#include <stdint.h>

#include "driverlib.h"

namespace ipc {

// Boot-time handshake: returns once the other core has called it with the same
// flag. Each core initialises its side of all channels before calling this, and
// uses them only afterwards.
void syncCores(IPC_Type_t link, uint16_t flag);

} // namespace ipc

#endif
