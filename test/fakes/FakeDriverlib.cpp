#include "FakeDriverlib.hpp"

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
