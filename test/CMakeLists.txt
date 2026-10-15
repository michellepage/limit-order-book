include( FetchContent )
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set( gtest_force_shared_crt ON CACHE BOOL "" FORCE )
FetchContent_MakeAvailable(googletest)

add_executable( obe_tests
    testBookSide.cpp
    testMatchingEngine.cpp
    testOrder.cpp
    testPriceLevel.cpp
)

target_link_libraries( obe_tests
    PRIVATE
    obe GTest::gtest_main
)

include( GoogleTest )
gtest_discover_tests( obe_tests )