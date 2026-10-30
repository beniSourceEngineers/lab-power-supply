# Host tests for the handwritten C++ code in ccs/ (AGENTS.md §5, §8).
#
#   cmake -S test -B build/test && cmake --build build/test && ctest --test-dir build/test
#
# Two parts:
#  - labpsu_production_cxx03: production sources compiled as C++03 with
#    -pedantic -Werror, to catch C++11 features that the C28x compiler rejects.
#  - labpsu_unit_tests: GoogleTest/GoogleMock unit tests (newer C++ allowed).
cmake_minimum_required(VERSION 3.20)
project(labpsu_host_tests CXX)
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

# Production code, built for the host against the fake driverlib.
add_library(labpsu_production_cxx03 STATIC
    ${CCS_DIR}/common/app/Blinker.cpp
    ${CCS_DIR}/common/hal/CpuTimer.cpp
    ${CCS_DIR}/common/hal/GpioOutput.cpp
)
target_include_directories(labpsu_production_cxx03 PUBLIC
    ${CCS_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/driverlib
)
target_compile_options(labpsu_production_cxx03 PRIVATE -std=c++03 -pedantic -Wall -Wextra -Werror)

add_executable(labpsu_unit_tests
    fakes/FakeDriverlib.cpp
    unit/BlinkerTest.cpp
    unit/CpuTimerTest.cpp
    unit/GpioOutputTest.cpp
)
target_include_directories(labpsu_unit_tests PRIVATE fakes mocks)
target_compile_features(labpsu_unit_tests PRIVATE cxx_std_17)
target_link_libraries(labpsu_unit_tests PRIVATE labpsu_production_cxx03 GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(labpsu_unit_tests)
