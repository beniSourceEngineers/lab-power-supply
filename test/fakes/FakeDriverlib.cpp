#include "FakeDriverlib.hpp"

#include <atomic>
#include <sstream>

#include "driverlib.h"

namespace fake {

std::vector<std::string>& driverlibCalls()
{
    static std::vector<std::string> calls;
    return calls;
}

} // namespace fake

namespace {

// One per direction, indexed by the IPC_Type_t of the sending core. Atomic flags,
// because the stress tests run the two sides in two threads.
struct IpcDirection {
    std::atomic<uint32_t> flags;
    uint32_t command;
    uint32_t addr;
    uint32_t data;
    uint32_t reply;
};

IpcDirection ipcDirections[IPC_TOTAL_NUM];

IpcDirection& localToRemote(IPC_Type_t ipcType) { return ipcDirections[ipcType]; }

IpcDirection& remoteToLocal(IPC_Type_t ipcType)
{
    return ipcDirections[ipcType == IPC_CPU1_L_CPU2_R ? IPC_CPU2_L_CPU1_R : IPC_CPU1_L_CPU2_R];
}

void record(const char* name, uint32_t a)
{
    std::ostringstream s;
    s << name << "(" << a << ")";
    fake::driverlibCalls().push_back(s.str());
}

void record(const char* name, uint32_t a, uint32_t b)
{
    std::ostringstream s;
    s << name << "(" << a << ", " << b << ")";
    fake::driverlibCalls().push_back(s.str());
}

} // namespace

void GPIO_writePin(uint32_t pin, uint32_t outVal) { record("GPIO_writePin", pin, outVal); }
void GPIO_togglePin(uint32_t pin) { record("GPIO_togglePin", pin); }

void CPUTimer_stopTimer(uint32_t base) { record("CPUTimer_stopTimer", base); }
void CPUTimer_startTimer(uint32_t base) { record("CPUTimer_startTimer", base); }
void CPUTimer_setPeriod(uint32_t base, uint32_t periodCount) { record("CPUTimer_setPeriod", base, periodCount); }
void CPUTimer_setPreScaler(uint32_t base, uint16_t prescaler) { record("CPUTimer_setPreScaler", base, prescaler); }
void CPUTimer_reloadTimerCounter(uint32_t base) { record("CPUTimer_reloadTimerCounter", base); }
void CPUTimer_setEmulationMode(uint32_t base, CPUTimer_EmulationMode mode) { record("CPUTimer_setEmulationMode", base, mode); }
void CPUTimer_enableInterrupt(uint32_t base) { record("CPUTimer_enableInterrupt", base); }

void fake::resetIpc()
{
    for (IpcDirection& direction : ipcDirections) {
        direction.flags = 0U;
        direction.command = 0U;
        direction.addr = 0U;
        direction.data = 0U;
        direction.reply = 0U;
    }
}

void IPC_setFlagLtoR(IPC_Type_t ipcType, uint32_t flags) { localToRemote(ipcType).flags |= flags; }
void IPC_clearFlagLtoR(IPC_Type_t ipcType, uint32_t flags) { localToRemote(ipcType).flags &= ~flags; }
void IPC_ackFlagRtoL(IPC_Type_t ipcType, uint32_t flags) { remoteToLocal(ipcType).flags &= ~flags; }
bool IPC_isFlagBusyLtoR(IPC_Type_t ipcType, uint32_t flags) { return (localToRemote(ipcType).flags & flags) != 0U; }
bool IPC_isFlagBusyRtoL(IPC_Type_t ipcType, uint32_t flags) { return (remoteToLocal(ipcType).flags & flags) != 0U; }

// Records only: the real one blocks until the other core has called it too.
void IPC_sync(IPC_Type_t ipcType, uint32_t flag) { record("IPC_sync", ipcType, flag); }

bool IPC_sendCommand(IPC_Type_t ipcType, uint32_t flags, bool, uint32_t command, uint32_t addr, uint32_t data)
{
    IpcDirection& direction = localToRemote(ipcType);
    if ((direction.flags & flags) != 0U) {
        return false;
    }
    direction.command = command;
    direction.addr = addr;
    direction.data = data;
    direction.flags |= flags;
    return true;
}

bool IPC_readCommand(IPC_Type_t ipcType, uint32_t flags, bool, uint32_t* command, uint32_t* addr, uint32_t* data)
{
    const IpcDirection& direction = remoteToLocal(ipcType);
    if ((direction.flags & flags) == 0U) {
        return false;
    }
    *command = direction.command;
    *addr = direction.addr;
    *data = direction.data;
    return true;
}

void IPC_sendResponse(IPC_Type_t ipcType, uint32_t data) { remoteToLocal(ipcType).reply = data; }
uint32_t IPC_getResponse(IPC_Type_t ipcType) { return localToRemote(ipcType).reply; }
