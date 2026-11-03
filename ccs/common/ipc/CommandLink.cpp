#include "ipc/CommandLink.hpp"

namespace ipc {

CommandLink::CommandLink(IPC_Type_t link, uint16_t flag)
    : link_(link)
    , flagMask_(1UL << flag)
{
}

void CommandLink::init()
{
    IPC_clearFlagLtoR(link_, flagMask_);
}

bool CommandLink::send(uint32_t command, uint32_t addr, uint32_t data)
{
    return IPC_sendCommand(link_, flagMask_, IPC_ADDR_CORRECTION_DISABLE, command, addr, data);
}

bool CommandLink::isBusy() const
{
    return IPC_isFlagBusyLtoR(link_, flagMask_);
}

uint32_t CommandLink::response() const
{
    return IPC_getResponse(link_);
}

bool CommandLink::poll(ICommandHandler& handler)
{
    uint32_t command = 0UL;
    uint32_t addr = 0UL;
    uint32_t data = 0UL;
    if (!IPC_readCommand(link_, flagMask_, IPC_ADDR_CORRECTION_DISABLE, &command, &addr, &data)) {
        return false;
    }

    // The response goes first: the sender reads it as soon as the flag clears.
    IPC_sendResponse(link_, handler.onCommand(command, addr, data));
    IPC_ackFlagRtoL(link_, flagMask_);
    return true;
}

} // namespace ipc
