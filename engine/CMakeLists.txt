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
    # Prefer the NDK selected by AGP's actual CMake toolchain. The machine
    # can have more than one NDK installed, so ANDROID_NDK_HOME is fallback only.
    set(GE_NDK_ROOT "")
    if(CMAKE_TOOLCHAIN_FILE)
        get_filename_component(GE_TOOLCHAIN_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
        get_filename_component(GE_NDK_BUILD_DIR "${GE_TOOLCHAIN_DIR}" DIRECTORY)
        get_filename_component(GE_NDK_FROM_TOOLCHAIN "${GE_NDK_BUILD_DIR}" DIRECTORY)
        if(EXISTS "${GE_NDK_FROM_TOOLCHAIN}/sources/android/native_app_glue/android_native_app_glue.c")
            set(GE_NDK_ROOT "${GE_NDK_FROM_TOOLCHAIN}")
        endif()
    endif()

    if(GE_NDK_ROOT STREQUAL "")
        foreach(GE_NDK_CANDIDATE
                "${ANDROID_NDK}"
                "${CMAKE_ANDROID_NDK}"
                "$ENV{ANDROID_NDK_HOME}"
                "$ENV{ANDROID_NDK_ROOT}")
            if(GE_NDK_CANDIDATE AND
               EXISTS "${GE_NDK_CANDIDATE}/sources/android/native_app_glue/android_native_app_glue.c")
                set(GE_NDK_ROOT "${GE_NDK_CANDIDATE}")
                break()
            endif()
        endforeach()
    endif()

    if(GE_NDK_ROOT STREQUAL "")
        message(FATAL_ERROR
            "Android NDK native_app_glue not found. "
            "CMAKE_TOOLCHAIN_FILE='${CMAKE_TOOLCHAIN_FILE}'. "
            "Checked the selected NDK root and fallback NDK environment paths.")
    endif()

    set(GE_NATIVE_APP_GLUE_DIR "${GE_NDK_ROOT}/sources/android/native_app_glue")
    add_library(native_app_glue STATIC
        "${GE_NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
    target_include_directories(native_app_glue PUBLIC "${GE_NATIVE_APP_GLUE_DIR}")

    add_library(globalengine SHARED
        src/platform/android_main.cpp
        src/platform/gles_renderer.cpp)
    target_include_directories(globalengine PRIVATE include src/platform "${GE_NATIVE_APP_GLUE_DIR}")
    target_compile_options(globalengine PRIVATE -Wall -Wextra -Wpedantic -O3)
    target_link_options(globalengine PRIVATE "-u" "ANativeActivity_onCreate")
    target_link_libraries(globalengine PRIVATE
        global_engine_core native_app_glue android log EGL GLESv3)
else()
    enable_testing()
    add_executable(global_engine_tests tests/engine_tests.cpp)
    target_link_libraries(global_engine_tests PRIVATE global_engine_core)
    add_test(NAME global_engine_core COMMAND global_engine_tests)
endif()
