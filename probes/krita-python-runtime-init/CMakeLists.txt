cmake_minimum_required(VERSION 3.21)
project(krita_python_runtime_init_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT PYTHON_ANDROID_INCLUDE_DIR)
    message(FATAL_ERROR "PYTHON_ANDROID_INCLUDE_DIR must point at target Python headers")
endif()

if(NOT PYTHON_ANDROID_LIBRARY)
    message(FATAL_ERROR "PYTHON_ANDROID_LIBRARY must point at target libpython")
endif()

add_library(krita_python_runtime_init_probe SHARED
    runtime_probe.cpp)

target_include_directories(krita_python_runtime_init_probe PRIVATE
    "${PYTHON_ANDROID_INCLUDE_DIR}")

target_link_libraries(krita_python_runtime_init_probe PRIVATE
    "${PYTHON_ANDROID_LIBRARY}"
    log)

target_link_options(krita_python_runtime_init_probe PRIVATE
    -Wl,-z,max-page-size=16384)
