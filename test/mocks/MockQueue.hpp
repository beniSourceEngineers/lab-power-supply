#ifndef TEST_MOCKQUEUE_HPP
#define TEST_MOCKQUEUE_HPP

#include <gmock/gmock.h>

#include "ipc/IQueue.hpp"

template <typename T>
class MockQueue : public ipc::IQueue<T> {
public:
    MOCK_METHOD(bool, push, (const T& element), (override));
    MOCK_METHOD(bool, pop, (T& out), (override));
};

#endif
