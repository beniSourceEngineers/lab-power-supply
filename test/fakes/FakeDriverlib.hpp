#ifndef TEST_FAKEDRIVERLIB_HPP
#define TEST_FAKEDRIVERLIB_HPP

#include <string>
#include <vector>

namespace fake {

// Every call into the fake driverlib, in order, e.g. "GPIO_writePin(31, 1)".
std::vector<std::string>& driverlibCalls();

} // namespace fake

#endif
