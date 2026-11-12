#ifndef TEST_MOCKTRIPZONE_HPP
#define TEST_MOCKTRIPZONE_HPP

#include <gmock/gmock.h>

#include "hal/ITripZone.hpp"

class MockTripZone : public hal::ITripZone {
public:
    MOCK_METHOD(void, forceTrip, (), (override));
    MOCK_METHOD(void, clearTrip, (), (override));
    MOCK_METHOD(bool, isTripped, (), (const, override));
};

#endif
