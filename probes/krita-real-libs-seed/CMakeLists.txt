cmake_minimum_required(VERSION 3.21)
project(krita_real_libs_seed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT KRITA_SOURCE_DIR)
    message(FATAL_ERROR "KRITA_SOURCE_DIR must point at an upstream Krita checkout")
endif()

if(NOT ANDROID_PREFIX)
    message(FATAL_ERROR "ANDROID_PREFIX must point at the Android dependency prefix")
endif()

if(NOT ANDROID_ABI AND CMAKE_ANDROID_ARCH_ABI)
    set(ANDROID_ABI "${CMAKE_ANDROID_ARCH_ABI}")
endif()

if(NOT ANDROID_ABI)
    message(FATAL_ERROR "ANDROID_ABI must be set")
endif()

set(QT_MAJOR_VERSION 5)

# Keep these values aligned with the upstream Krita 5 branch constants.  The
# probe builds upstream library CMakeLists directly, without configuring all of
# Krita's top-level project.
set(KRITA_VERSION_STRING "5.4.0-prealpha")
set(KRITA_STABLE_VERSION_MAJOR 5)
set(KRITA_STABLE_VERSION_MINOR 4)
set(KRITA_VERSION_RELEASE 0)
set(KRITA_VERSION_REVISION 0)
set(KRITA_ALPHA 1)
set(KRITA_GIT_SHA1_STRING "")
set(KRITA_GIT_BRANCH_STRING "")
set(GENERIC_KRITA_LIB_VERSION "20.0.0")
set(GENERIC_KRITA_LIB_SOVERSION "20")

include(GNUInstallDirs)
include(GenerateExportHeader)

set(KDE_INSTALL_LIBDIR "lib")
set(INSTALL_TARGETS_DEFAULT_ARGS
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

macro(kis_add_library)
    add_library(${ARGV})
endmacro()

function(add_android_qt5_module module)
    set(library_path "${ANDROID_PREFIX}/lib/libQt5${module}_${ANDROID_ABI}.so")
    set(include_root "${ANDROID_PREFIX}/include")
    set(module_include "${ANDROID_PREFIX}/include/Qt${module}")

    if(NOT EXISTS "${library_path}")
        message(FATAL_ERROR "Missing Android Qt5${module} library: ${library_path}")
    endif()

    if(NOT IS_DIRECTORY "${module_include}")
        message(FATAL_ERROR "Missing Android Qt5${module} headers: ${module_include}")
    endif()

    string(TOUPPER "${module}" module_upper)
    add_library(Qt5::${module} SHARED IMPORTED GLOBAL)
    set_target_properties(Qt5::${module} PROPERTIES
        IMPORTED_LOCATION "${library_path}"
        INTERFACE_INCLUDE_DIRECTORIES "${include_root};${module_include}"
        INTERFACE_COMPILE_DEFINITIONS "QT_${module_upper}_LIB;QT_NO_DEBUG")
endfunction()

add_android_qt5_module(Core)

set(_kritaversion_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaversion")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/version" "${_kritaversion_build_dir}")

target_include_directories(kritaversion
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/version>"
        "$<BUILD_INTERFACE:${_kritaversion_build_dir}>")
