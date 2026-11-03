#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "FakeDriverlib.hpp"
#include "ipc/SyncCores.hpp"

using ::testing::ElementsAre;

TEST(SyncCoresTest, ConvertsFlagNumberToMask)
{
    fake::driverlibCalls().clear();
    ipc::syncCores(IPC_CPU2_L_CPU1_R, 31U);
    EXPECT_THAT(fake::driverlibCalls(), ElementsAre("IPC_sync(1, 2147483648)"));
}
