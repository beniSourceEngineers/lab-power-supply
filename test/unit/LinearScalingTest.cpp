#include <gtest/gtest.h>

#include "app/LinearScaling.hpp"

TEST(LinearScalingTest, AppliesGainAndOffset)
{
    const app::LinearScaling scaling = { 0.5F, -10.0F };
    EXPECT_FLOAT_EQ(scaling.apply(0U), -10.0F);
    EXPECT_FLOAT_EQ(scaling.apply(100U), 40.0F);
}

TEST(LinearScalingTest, ProjectAScalingMapsFullScaleToFiveVolts)
{
    const app::LinearScaling scaling = { 5.0F / 4096.0F, 0.0F };
    EXPECT_FLOAT_EQ(scaling.apply(4096U), 5.0F);
    EXPECT_FLOAT_EQ(scaling.apply(2048U), 2.5F);
}
