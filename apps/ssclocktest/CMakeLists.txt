# Build the Second Sight clock/bus contract check from main.cpp and the production
# card's test-only exact-stop library. CTest invokes it; assertions determine success.
# This file describes build wiring, not a hardware rule.
add_executable(ssclocktest main.cpp)

target_link_libraries(ssclocktest PRIVATE
    gs2_devices_secondsight_test
)

# Assertions are the test result, including in Release builds.
target_compile_options(ssclocktest PRIVATE -UNDEBUG)

add_test(NAME ssclocktest COMMAND ssclocktest)
