#ifndef IPC_ICOMMANDHANDLER_HPP
#define IPC_ICOMMANDHANDLER_HPP

#include <stdint.h>

namespace ipc {

// Handles a command from the other core and returns the response value.
class ICommandHandler {
public:
    virtual uint32_t onCommand(uint32_t command, uint32_t addr, uint32_t data) = 0;

protected:
    ~ICommandHandler() {}
};

} // namespace ipc

#endif
