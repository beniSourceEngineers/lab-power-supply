// Host stand-in for C2000Ware driverlib.h. Declares only the driverlib
// functions that the HAL classes use; FakeDriverlib.cpp records the calls.
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

void GPIO_writePin(uint32_t pin, uint32_t outVal);
void GPIO_togglePin(uint32_t pin);

void CPUTimer_stopTimer(uint32_t base);
void CPUTimer_startTimer(uint32_t base);
void CPUTimer_setPeriod(uint32_t base, uint32_t periodCount);
void CPUTimer_setPreScaler(uint32_t base, uint16_t prescaler);
void CPUTimer_reloadTimerCounter(uint32_t base);
void CPUTimer_setEmulationMode(uint32_t base, CPUTimer_EmulationMode mode);
void CPUTimer_enableInterrupt(uint32_t base);

#endif
