#ifndef IPC_ICOMMANDLINK_HPP
#define IPC_ICOMMANDLINK_HPP

#include <stdint.h>

#include "ipc/ICommandHandler.hpp"

namespace ipc {

// Commands with acknowledge and a response. One command per direction can be
// in flight.
class ICommandLink {
public:
    // Returns false if the previous command is not acknowledged yet; the command
    // is not sent.
    virtual bool send(uint32_t command, uint32_t addr, uint32_t data) = 0;

    // True until the other core has handled the last command.
    virtual bool isBusy() const = 0;

    // Response to the last command, valid once isBusy() is false.
    virtual uint32_t response() const = 0;

    // Passes a pending command from the other core to the handler, sends the
    // response and acknowledges. Returns false if none is pending. Call it from
    // the main loop or from the IPC ISR.
    virtual bool poll(ICommandHandler& handler) = 0;

protected:
    ~ICommandLink() {}
};

} // namespace ipc

#endif
