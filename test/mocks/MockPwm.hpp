#ifndef TEST_MOCKPWM_HPP
#define TEST_MOCKPWM_HPP

#include <gmock/gmock.h>

#include "hal/IPwm.hpp"

class MockPwm : public hal::IPwm {
public:
    MOCK_METHOD(void, setDuty, (float duty), (override));
};

#endif
