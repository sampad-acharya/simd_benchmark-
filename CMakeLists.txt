cmake_minimum_required(VERSION 3.16)
project(simd_benchmark C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)

add_executable(avx2_bench main.cpp)

target_compile_options(avx2_bench PRIVATE
    -O3
    -march=native
    -mprefer-vector-width=256
    -ffast-math
)

target_link_libraries(avx2_bench PRIVATE m)

# Google Benchmark target
include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(avx2_bench_gbench benchmark.cpp)

set_target_properties(avx2_bench_gbench PROPERTIES CXX_STANDARD 17)

target_compile_options(avx2_bench_gbench PRIVATE
    -O3
    -march=native
    -ffast-math
)

target_link_libraries(avx2_bench_gbench PRIVATE benchmark::benchmark pthread)
