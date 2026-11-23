cmake_minimum_required(VERSION 3.22.1)
project(GlobalEngine VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(global_engine_core STATIC src/world.cpp)
target_include_directories(global_engine_core PUBLIC include)
target_compile_options(global_engine_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wconversion>
)

if(ANDROID)
    set(GE_NDK_ROOT "")
    if(DEFINED CMAKE_ANDROID_NDK)
        set(GE_NDK_ROOT "\${CMAKE_ANDROID_NDK}")
    elseif(DEFINED ANDROID_NDK)
        set(GE_NDK_ROOT "\${ANDROID_NDK}")
    elseif(DEFINED ENV{ANDROID_NDK_HOME})
        set(GE_NDK_ROOT "$ENV{ANDROID_NDK_HOME}")
    elseif(DEFINED ENV{ANDROID_NDK_ROOT})
        set(GE_NDK_ROOT "$ENV{ANDROID_NDK_ROOT}")
    endif()
    set(NATIVE_APP_GLUE_DIR "\${GE_NDK_ROOT}/sources/android/native_app_glue")
    if(GE_NDK_ROOT STREQUAL "" OR NOT EXISTS "\${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
        message(FATAL_ERROR "Android NDK native_app_glue was not found")
    endif()

    add_library(native_app_glue STATIC
        "\${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
    target_include_directories(native_app_glue PUBLIC "\${NATIVE_APP_GLUE_DIR}")

    add_library(globalengine SHARED
        src/platform/android_main.cpp
        src/platform/gles_renderer.cpp)
    target_include_directories(globalengine PRIVATE include src/platform "\${NATIVE_APP_GLUE_DIR}")
    target_compile_options(globalengine PRIVATE -Wall -Wextra -Wpedantic -O3)
    target_link_libraries(globalengine PRIVATE
        global_engine_core native_app_glue android log EGL GLESv3)
else()
    enable_testing()
    add_executable(global_engine_tests tests/engine_tests.cpp)
    target_link_libraries(global_engine_tests PRIVATE global_engine_core)
    add_test(NAME global_engine_core COMMAND global_engine_tests)
endif()
