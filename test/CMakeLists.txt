# Host tests for the handwritten C++ code in ccs/ (AGENTS.md §5, §8).
#
#   cmake -S test -B build/test && cmake --build build/test && ctest --test-dir build/test
#
# Three parts:
#  - labpsu_control_generated: the generated control code (control/generated/),
#    compiled as C. Its headers are SYSTEM includes: they use `long long`,
#    which -pedantic rejects in C++03, and they must not be edited.
#  - labpsu_production_cxx03: production sources compiled as C++03 with
#    -pedantic -Werror, to catch C++11 features that the C28x compiler rejects.
#    Templates are instantiated in cxx03/, as they are only compiled when used.
#  - labpsu_unit_tests: GoogleTest/GoogleMock unit tests (newer C++ allowed).
cmake_minimum_required(VERSION 3.20)
project(labpsu_host_tests C CXX)
enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CCS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ccs)
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../control/generated)

# Generated control code, unchanged.
add_library(labpsu_control_generated STATIC
    ${GENERATED_DIR}/Current_Control/Current_Control.c
    ${GENERATED_DIR}/Voltage_Control/Voltage_Control.c
)
target_include_directories(labpsu_control_generated SYSTEM PUBLIC
    ${GENERATED_DIR}/shared
    ${GENERATED_DIR}/Current_Control
    ${GENERATED_DIR}/Voltage_Control
)
target_link_libraries(labpsu_control_generated PUBLIC m)

# Production code, built for the host against the fake driverlib.
add_library(labpsu_production_cxx03 STATIC
    ${CCS_DIR}/common/app/Blinker.cpp
    ${CCS_DIR}/common/hal/CpuTimer.cpp
    ${CCS_DIR}/common/hal/GpioOutput.cpp
    ${CCS_DIR}/common/ipc/CommandLink.cpp
    ${CCS_DIR}/common/ipc/SyncCores.cpp
    ${CCS_DIR}/cpu1/src/app/OpenLoopTask.cpp
    ${CCS_DIR}/cpu1/src/ctrl/CurrentLoop.cpp
    ${CCS_DIR}/cpu1/src/ctrl/VoltageLoop.cpp
    ${CCS_DIR}/cpu1/src/hal/Adc.cpp
    ${CCS_DIR}/cpu1/src/hal/AdcInput.cpp
    ${CCS_DIR}/cpu1/src/hal/Cmpss.cpp
    ${CCS_DIR}/cpu1/src/hal/EpwmTimeBase.cpp
    ${CCS_DIR}/cpu1/src/hal/PowerStagePwm.cpp
    ${CCS_DIR}/cpu1/src/hal/TripZone.cpp
    cxx03/IpcTemplates.cpp
)
target_include_directories(labpsu_production_cxx03 PUBLIC
    ${CCS_DIR}/common
    ${CCS_DIR}/cpu1/src
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/driverlib
)
target_link_libraries(labpsu_production_cxx03 PUBLIC labpsu_control_generated)
target_compile_options(labpsu_production_cxx03 PRIVATE -std=c++03 -pedantic -Wall -Wextra -Werror)

add_executable(labpsu_unit_tests
    fakes/FakeDriverlib.cpp
    unit/AdcInputTest.cpp
    unit/AdcTest.cpp
    unit/AppMocksTest.cpp
    unit/BlinkerTest.cpp
    unit/CmpssTest.cpp
    unit/CommandLinkTest.cpp
    unit/CpuTimerTest.cpp
    unit/CtrlMocksTest.cpp
    unit/CurrentLoopTest.cpp
    unit/EpwmTimeBaseTest.cpp
    unit/GpioOutputTest.cpp
    unit/IpcMocksTest.cpp
    unit/LinearScalingTest.cpp
    unit/MailboxStressTest.cpp
    unit/MailboxTest.cpp
    unit/OpenLoopTaskTest.cpp
    unit/PowerStagePwmTest.cpp
    unit/QueueStressTest.cpp
    unit/QueueTest.cpp
    unit/SyncCoresTest.cpp
    unit/TripZoneTest.cpp
    unit/VoltageLoopTest.cpp
)
target_include_directories(labpsu_unit_tests PRIVATE fakes mocks)
target_compile_features(labpsu_unit_tests PRIVATE cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(labpsu_unit_tests PRIVATE labpsu_production_cxx03 GTest::gmock_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(labpsu_unit_tests)
