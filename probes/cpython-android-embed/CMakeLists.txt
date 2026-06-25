cmake_minimum_required(VERSION 3.22)

project(cpython_android_embed_probe LANGUAGES C CXX)

set(PYTHON_ANDROID_PREFIX "" CACHE PATH "Path to the Android Python package prefix")
set(PYTHON_ANDROID_VERSION "3.14" CACHE STRING "Android Python major.minor version")

if(NOT ANDROID)
    message(FATAL_ERROR "This probe must be built with the Android NDK toolchain.")
endif()

if(NOT PYTHON_ANDROID_PREFIX)
    message(FATAL_ERROR "Set -DPYTHON_ANDROID_PREFIX=/path/to/python-android/prefix")
endif()

set(PYTHON_ANDROID_INCLUDE
    "${PYTHON_ANDROID_PREFIX}/include/python${PYTHON_ANDROID_VERSION}")
set(PYTHON_ANDROID_LIBRARY
    "${PYTHON_ANDROID_PREFIX}/lib/libpython${PYTHON_ANDROID_VERSION}.so")

if(NOT EXISTS "${PYTHON_ANDROID_INCLUDE}/Python.h")
    message(FATAL_ERROR "Python.h was not found in ${PYTHON_ANDROID_INCLUDE}")
endif()

if(NOT EXISTS "${PYTHON_ANDROID_LIBRARY}")
    message(FATAL_ERROR "libpython was not found: ${PYTHON_ANDROID_LIBRARY}")
endif()

find_library(ANDROID_LOG_LIBRARY log REQUIRED)
find_library(ANDROID_DL_LIBRARY dl REQUIRED)
find_library(ANDROID_M_LIBRARY m REQUIRED)

add_library(krita_android_python_probe SHARED probe.cpp)

target_include_directories(krita_android_python_probe PRIVATE
    "${PYTHON_ANDROID_INCLUDE}")

target_link_libraries(krita_android_python_probe PRIVATE
    "${PYTHON_ANDROID_LIBRARY}"
    "${ANDROID_LOG_LIBRARY}"
    "${ANDROID_DL_LIBRARY}"
    "${ANDROID_M_LIBRARY}")

set_target_properties(krita_android_python_probe PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

add_custom_command(TARGET krita_android_python_probe POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PYTHON_ANDROID_LIBRARY}"
        "$<TARGET_FILE_DIR:krita_android_python_probe>/libpython${PYTHON_ANDROID_VERSION}.so")
