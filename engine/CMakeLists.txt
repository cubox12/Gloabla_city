cmake_minimum_required(VERSION 3.22.1)
project(GlobalEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(global_engine_core STATIC src/world.cpp)
target_include_directories(global_engine_core PUBLIC include)
target_compile_options(global_engine_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>
)

enable_testing()
add_executable(global_engine_tests tests/engine_tests.cpp)
target_link_libraries(global_engine_tests PRIVATE global_engine_core)
add_test(NAME global_engine_core COMMAND global_engine_tests)
