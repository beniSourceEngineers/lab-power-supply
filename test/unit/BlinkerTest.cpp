#include <gtest/gtest.h>

#include "MockDigitalOutput.hpp"
#include "app/Blinker.hpp"

using ::testing::StrictMock;

TEST(BlinkerTest, DoesNotToggleBeforePeriod)
{
    StrictMock<MockDigitalOutput> output;
    app::Blinker blinker(output, 3U);

    blinker.tick();
    blinker.tick();
}

TEST(BlinkerTest, TogglesOncePerPeriod)
{
    StrictMock<MockDigitalOutput> output;
    app::Blinker blinker(output, 3U);

    EXPECT_CALL(output, toggle()).Times(2);
    for (int i = 0; i < 7; ++i) {
        blinker.tick();
    }
}

TEST(BlinkerTest, TogglesEveryTickWithPeriodOne)
{
    StrictMock<MockDigitalOutput> output;
    app::Blinker blinker(output, 1U);

    EXPECT_CALL(output, toggle()).Times(3);
    blinker.tick();
    blinker.tick();
    blinker.tick();
}
