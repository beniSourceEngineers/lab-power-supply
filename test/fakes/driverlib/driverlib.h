// Host stand-in for C2000Ware driverlib.h. Declares only the driverlib
// functions that the HAL and IPC classes use; FakeDriverlib.cpp implements them.
// Production code includes this header with -std=c++03 -pedantic, so keep it
// C++03.
#ifndef FAKE_DRIVERLIB_H
#define FAKE_DRIVERLIB_H

#include <stdint.h>

typedef enum
{
    CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT = 0x0000,
    CPUTIMER_EMULATIONMODE_STOPATZERO = 0x0400,
    CPUTIMER_EMULATIONMODE_RUNFREE = 0x0800
} CPUTimer_EmulationMode;

// Same names and meaning as driverlib ipc.h. Index 0 is the direction from CPU1
// to CPU2, as seen from IPC_CPU1_L_CPU2_R.
typedef enum
{
    IPC_CPU1_L_CPU2_R,
    IPC_CPU2_L_CPU1_R,
    IPC_TOTAL_NUM
} IPC_Type_t;

#define IPC_ADDR_CORRECTION_ENABLE  true
#define IPC_ADDR_CORRECTION_DISABLE false

void GPIO_writePin(uint32_t pin, uint32_t outVal);
void GPIO_togglePin(uint32_t pin);

void CPUTimer_stopTimer(uint32_t base);
void CPUTimer_startTimer(uint32_t base);
void CPUTimer_setPeriod(uint32_t base, uint32_t periodCount);
void CPUTimer_setPreScaler(uint32_t base, uint16_t prescaler);
void CPUTimer_reloadTimerCounter(uint32_t base);
void CPUTimer_setEmulationMode(uint32_t base, CPUTimer_EmulationMode mode);
void CPUTimer_enableInterrupt(uint32_t base);

void IPC_setFlagLtoR(IPC_Type_t ipcType, uint32_t flags);
void IPC_clearFlagLtoR(IPC_Type_t ipcType, uint32_t flags);
void IPC_ackFlagRtoL(IPC_Type_t ipcType, uint32_t flags);
bool IPC_isFlagBusyLtoR(IPC_Type_t ipcType, uint32_t flags);
bool IPC_isFlagBusyRtoL(IPC_Type_t ipcType, uint32_t flags);
void IPC_sync(IPC_Type_t ipcType, uint32_t flag);
bool IPC_sendCommand(IPC_Type_t ipcType, uint32_t flags, bool addrCorrEnable,
                     uint32_t command, uint32_t addr, uint32_t data);
bool IPC_readCommand(IPC_Type_t ipcType, uint32_t flags, bool addrCorrEnable,
                     uint32_t* command, uint32_t* addr, uint32_t* data);
void IPC_sendResponse(IPC_Type_t ipcType, uint32_t data);
uint32_t IPC_getResponse(IPC_Type_t ipcType);

#endif
