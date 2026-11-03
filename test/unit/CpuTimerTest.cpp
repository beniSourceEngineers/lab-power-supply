#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "hal/CpuTimer.hpp"

using ::testing::ElementsAre;

namespace {
const uint32_t kBase = 0x0C00U;            // CPUTIMER0_BASE
const uint32_t kSysClkHz = 200000000UL;    // 200 MHz
}

class CpuTimerTest : public ::testing::Test {
protected:
    void SetUp() override { fake::driverlibCalls().clear(); }
};

TEST_F(CpuTimerTest, ConfigurePeriodicSetsUpStoppedTimerWithInterrupt)
{
    hal::CpuTimer timer(kBase, kSysClkHz);
    timer.configurePeriodic(1000U);

    EXPECT_THAT(fake::driverlibCalls(), ElementsAre(
        "CPUTimer_stopTimer(3072)",
        "CPUTimer_setPeriod(3072, 199999)",   // 200 ticks/us * 1000 us - 1
        "CPUTimer_setPreScaler(3072, 0)",
        "CPUTimer_reloadTimerCounter(3072)",
        "CPUTimer_setEmulationMode(3072, 0)",
        "CPUTimer_enableInterrupt(3072)"));
}

TEST_F(CpuTimerTest, StartStartsTimer)
{
    hal::CpuTimer timer(kBase, kSysClkHz);
    timer.start();
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre("CPUTimer_startTimer(3072)"));
}
