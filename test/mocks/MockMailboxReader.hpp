#ifndef TEST_MOCKMAILBOXREADER_HPP
#define TEST_MOCKMAILBOXREADER_HPP

#include <gmock/gmock.h>

#include "ipc/IMailboxReader.hpp"

template <typename T>
class MockMailboxReader : public ipc::IMailboxReader<T> {
public:
    MOCK_METHOD(bool, tryRead, (T& out), (override));
    MOCK_METHOD(bool, hasNew, (), (const, override));
};

#endif
