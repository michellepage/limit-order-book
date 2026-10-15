include ( FetchContent )
FetchContent_Declare(
    benchmarks
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)

set( BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE )
set( BENCHMARK_ENABLE_GTESTS_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable( benchmarks )

add_executable( obe_benchmark
    benchMatchingEngine.cpp
)

target_link_libraries( obe_benchmark
    PRIVATE
    obe benchmark::benchmark_main
)