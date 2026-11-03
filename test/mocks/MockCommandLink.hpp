#ifndef TEST_MOCKCOMMANDLINK_HPP
#define TEST_MOCKCOMMANDLINK_HPP

#include <gmock/gmock.h>

#include "ipc/ICommandLink.hpp"

class MockCommandLink : public ipc::ICommandLink {
public:
    MOCK_METHOD(bool, send, (uint32_t command, uint32_t addr, uint32_t data), (override));
    MOCK_METHOD(bool, isBusy, (), (const, override));
    MOCK_METHOD(uint32_t, response, (), (const, override));
    MOCK_METHOD(bool, poll, (ipc::ICommandHandler& handler), (override));
};

#endif
