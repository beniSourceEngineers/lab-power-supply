#ifndef TEST_FAKEDRIVERLIB_HPP
#define TEST_FAKEDRIVERLIB_HPP

#include <stdint.h>

#include <initializer_list>
#include <string>
#include <vector>

namespace fake {

// Every call into the fake driverlib, in order, e.g. "GPIO_writePin(31, 1)".
std::vector<std::string>& driverlibCalls();

// A call in the format of driverlibCalls(), built from driverlib names, e.g.
// call("ADC_enableConverter", {ADCA_BASE}).
std::string call(const char* name, std::initializer_list<uint32_t> args);

// The IPC registers behave as on the device, for both directions: flags set by
// one core are seen by the other. Clears all flags and command registers.
void resetIpc();

// Values returned by the driverlib read functions, which are not recorded in
// driverlibCalls(). resetPeripherals() sets them all back to 0 / false.
void setAdcResult(uint32_t resultBase, uint16_t soc, uint16_t value);
void setAdcInterruptOverflow(bool overflow);
void setTripZoneFlags(uint32_t base, uint16_t flags);
void resetPeripherals();

} // namespace fake

#endif
