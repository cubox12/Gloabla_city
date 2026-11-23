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
    # Resolve native_app_glue from the NDK root without assuming which
    # NDK variable the Android Gradle Plugin populated.
    find_path(GE_NATIVE_APP_GLUE_DIR
        NAMES android_native_app_glue.h
        HINTS
            "${CMAKE_ANDROID_NDK}"
            "${ANDROID_NDK}"
            "$ENV{ANDROID_NDK_HOME}"
            "$ENV{ANDROID_NDK_ROOT}"
        PATH_SUFFIXES sources/android/native_app_glue
        NO_DEFAULT_PATH
    )
    if(NOT GE_NATIVE_APP_GLUE_DIR OR
       NOT EXISTS "${GE_NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
        message(FATAL_ERROR
            "native_app_glue files not found. "
            "CMAKE_ANDROID_NDK='${CMAKE_ANDROID_NDK}', "
            "ANDROID_NDK='${ANDROID_NDK}', "
            "ANDROID_NDK_HOME='$ENV{ANDROID_NDK_HOME}', "
            "ANDROID_NDK_ROOT='$ENV{ANDROID_NDK_ROOT}'. "
            "Checked NDK sources/android/native_app_glue.")
    endif()

    add_library(native_app_glue STATIC
        "${GE_NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
    target_include_directories(native_app_glue PUBLIC "${GE_NATIVE_APP_GLUE_DIR}")

    add_library(globalengine SHARED
        src/platform/android_main.cpp
        src/platform/gles_renderer.cpp)
    target_include_directories(globalengine PRIVATE include src/platform "${GE_NATIVE_APP_GLUE_DIR}")
    target_compile_options(globalengine PRIVATE -Wall -Wextra -Wpedantic -O3)
    target_link_libraries(globalengine PRIVATE
        global_engine_core native_app_glue android log EGL GLESv3)
else()
    enable_testing()
    add_executable(global_engine_tests tests/engine_tests.cpp)
    target_link_libraries(global_engine_tests PRIVATE global_engine_core)
    add_test(NAME global_engine_core COMMAND global_engine_tests)
endif()
