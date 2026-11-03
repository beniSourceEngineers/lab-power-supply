#ifndef IPC_COMMANDLINK_HPP
#define IPC_COMMANDLINK_HPP

#include <stdint.h>

#include "driverlib.h"

#include "ipc/ICommandLink.hpp"

namespace ipc {

// ICommandLink on the driverlib IPC command registers. Both cores use the same
// flag number; each direction has its own flag and registers.
class CommandLink : public ICommandLink {
public:
    CommandLink(IPC_Type_t link, uint16_t flag);

    // Before syncCores(): clears a flag left over from before a reset.
    void init();

    virtual bool send(uint32_t command, uint32_t addr, uint32_t data);
    virtual bool isBusy() const;
    virtual uint32_t response() const;
    virtual bool poll(ICommandHandler& handler);

private:
    const IPC_Type_t link_;
    const uint32_t flagMask_;
};

} // namespace ipc

#endif
