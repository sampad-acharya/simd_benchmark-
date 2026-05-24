cmake_minimum_required(VERSION 3.16)
project(simd_benchmark C CXX)

set(CMAKE_CXX_STANDARD 11)

add_executable(avx2_bench main.cpp)

target_compile_options(avx2_bench PRIVATE
    -O3
    -march=native
    -mprefer-vector-width=256
    -ffast-math
)

target_link_libraries(avx2_bench PRIVATE m)
