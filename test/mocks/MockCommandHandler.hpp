#ifndef TEST_MOCKCOMMANDHANDLER_HPP
#define TEST_MOCKCOMMANDHANDLER_HPP

#include <gmock/gmock.h>

#include "ipc/ICommandHandler.hpp"

class MockCommandHandler : public ipc::ICommandHandler {
public:
    MOCK_METHOD(uint32_t, onCommand, (uint32_t command, uint32_t addr, uint32_t data), (override));
};

#endif
