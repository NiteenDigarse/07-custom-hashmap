cmake_minimum_required(VERSION 3.20)

project(CustomHashMap
    VERSION 1.0
    LANGUAGES CXX
)


add_executable(
    benchmark_custom_hashmap
    benchmarks/benchmark_custom_hashmap.cpp
)

target_include_directories(
    benchmark_custom_hashmap
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(
    benchmark_custom_hashmap
    PRIVATE
    cxx_std_17
)
