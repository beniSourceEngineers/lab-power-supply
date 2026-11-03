#ifndef TEST_MOCKMAILBOXWRITER_HPP
#define TEST_MOCKMAILBOXWRITER_HPP

#include <gmock/gmock.h>

#include "ipc/IMailboxWriter.hpp"

template <typename T>
class MockMailboxWriter : public ipc::IMailboxWriter<T> {
public:
    MOCK_METHOD(void, post, (const T& msg), (override));
};

#endif
