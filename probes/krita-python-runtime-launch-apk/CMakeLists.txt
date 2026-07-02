cmake_minimum_required(VERSION 3.21)
project(krita_python_runtime_launch_apk_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT KRITA_RUNTIME_INIT_PROBE_LIBRARY)
    message(FATAL_ERROR "KRITA_RUNTIME_INIT_PROBE_LIBRARY must point at libkrita_python_runtime_init_probe.so")
endif()

add_library(krita_python_runtime_init_probe SHARED IMPORTED)
set_target_properties(krita_python_runtime_init_probe PROPERTIES
    IMPORTED_LOCATION "${KRITA_RUNTIME_INIT_PROBE_LIBRARY}")

add_library(krita_python_runtime_launcher SHARED
    launcher.cpp)

target_link_libraries(krita_python_runtime_launcher PRIVATE
    krita_python_runtime_init_probe
    dl
    log)

target_link_options(krita_python_runtime_launcher PRIVATE
    -Wl,--no-undefined
    -Wl,-z,max-page-size=16384)
