cmake_minimum_required(VERSION 3.21)
project(krita_real_libs_seed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

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
set(KF_MAJOR 5)

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
include(CheckFunctionExists)

set(KDE_INSTALL_LIBDIR "lib")
set(INSTALL_TARGETS_DEFAULT_ARGS
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

macro(kis_add_library)
    add_library(${ARGV})
endmacro()

macro(kis_add_tests)
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
add_android_qt5_module(Gui)
add_android_qt5_module(Widgets)
add_android_qt5_module(Xml)
add_android_qt5_module(AndroidExtras)

find_program(_qt5_moc_executable
    NAMES moc
    HINTS "${ANDROID_PREFIX}/bin"
    NO_DEFAULT_PATH)

if(NOT _qt5_moc_executable)
    message(FATAL_ERROR "Could not find host Qt moc under ${ANDROID_PREFIX}/bin")
endif()

if(NOT TARGET Qt5::moc)
    add_executable(Qt5::moc IMPORTED GLOBAL)
    set_target_properties(Qt5::moc PROPERTIES
        IMPORTED_LOCATION "${_qt5_moc_executable}")
endif()

set(Qt5Core_VERSION_MAJOR 5)
set(Qt5Core_VERSION_MINOR 15)
set(Qt5Core_MOC_EXECUTABLE "${_qt5_moc_executable}")
set_property(TARGET Qt5::Core PROPERTY INTERFACE_QT_MAJOR_VERSION 5)

set(_external_include_dir "${CMAKE_CURRENT_BINARY_DIR}/generated-externals")
file(MAKE_DIRECTORY
    "${_external_include_dir}"
    "${_external_include_dir}/lager"
    "${_external_include_dir}/zug/compat"
    "${_external_include_dir}/zug/reducing"
    "${_external_include_dir}/zug/transducer"
    "${_external_include_dir}/unwindstack")

if(EXISTS "/usr/include/boost")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
        "/usr/include/boost" "${_external_include_dir}/boost")
endif()

file(WRITE "${_external_include_dir}/klocalizedstring.h" [=[
#pragma once

#include <QString>
#include <QStringList>

class KLocalizedString
{
public:
    KLocalizedString() = default;
    explicit KLocalizedString(const char *text)
        : m_text(QString::fromUtf8(text ? text : ""))
    {
    }
    explicit KLocalizedString(const QString &text)
        : m_text(text)
    {
    }

    QString toString() const
    {
        return m_text;
    }

    bool isEmpty() const
    {
        return m_text.isEmpty();
    }

    operator QString() const
    {
        return toString();
    }

    template <typename... Args>
    KLocalizedString subs(Args &&...) const
    {
        return *this;
    }

    static QStringList languages()
    {
        return QStringList();
    }

    static QString removeAcceleratorMarker(const QString &text)
    {
        return text;
    }

private:
    QString m_text;
};

inline KLocalizedString ki18n(const char *text)
{
    return KLocalizedString(text);
}

inline KLocalizedString ki18nc(const char *, const char *text)
{
    return KLocalizedString(text);
}

template <typename... Args>
inline QString i18n(const char *text, Args &&...)
{
    return QString::fromUtf8(text ? text : "");
}

template <typename... Args>
inline QString i18nc(const char *, const char *text, Args &&...)
{
    return QString::fromUtf8(text ? text : "");
}

template <typename... Args>
inline QString i18np(const char *singular, const char *, Args &&...)
{
    return QString::fromUtf8(singular ? singular : "");
}

template <typename... Args>
inline QString i18ncp(const char *, const char *singular, const char *, Args &&...)
{
    return QString::fromUtf8(singular ? singular : "");
}
]=])
file(COPY_FILE "${_external_include_dir}/klocalizedstring.h" "${_external_include_dir}/KLocalizedString")

file(WRITE "${_external_include_dir}/kconfig.h" [=[
#pragma once

class KConfig
{
};
]=])

file(WRITE "${_external_include_dir}/ksharedconfig.h" [=[
#pragma once

#include <QSharedPointer>
#include "kconfig.h"

class KSharedConfig : public KConfig
{
public:
    using Ptr = QSharedPointer<KSharedConfig>;

    static Ptr openConfig()
    {
        return Ptr::create();
    }
};
]=])

file(WRITE "${_external_include_dir}/kconfiggroup.h" [=[
#pragma once

#include <QString>
#include <QSharedPointer>
#include "ksharedconfig.h"

class KConfigGroup
{
public:
    KConfigGroup(const KSharedConfig::Ptr &, const char *)
    {
    }

    bool readEntry(const QString &, bool defaultValue) const
    {
        return defaultValue;
    }

    void writeEntry(const QString &, bool)
    {
    }
};
]=])

file(WRITE "${_external_include_dir}/lager/reader.hpp" [=[
#pragma once

namespace lager {

template <typename T>
class reader
{
};

template <typename... Args>
class with_result
{
public:
    template <typename Func>
    auto map(Func &&func) const
    {
        return func;
    }
};

template <typename... Args>
with_result<Args...> with(Args &&...)
{
    return {};
}

} // namespace lager
]=])

file(WRITE "${_external_include_dir}/lager/lenses.hpp" [=[
#pragma once

namespace lager {
namespace lenses {

template <typename Getter, typename Setter>
struct getset_lens
{
    Getter getter;
    Setter setter;
};

template <typename Getter, typename Setter>
getset_lens<Getter, Setter> getset(Getter getter, Setter setter)
{
    return {getter, setter};
}

} // namespace lenses
} // namespace lager
]=])

file(WRITE "${_external_include_dir}/zug/compat/invoke.hpp" [=[
#pragma once

#include <functional>
#include <utility>

namespace zug {
namespace compat {

template <typename Func, typename... Args>
decltype(auto) invoke(Func &&func, Args &&...args)
{
    return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
}

} // namespace compat
} // namespace zug
]=])

file(WRITE "${_external_include_dir}/zug/reducing/last.hpp" [=[
#pragma once

namespace zug {

struct last_t
{
};

inline constexpr last_t last {};

} // namespace zug
]=])

file(WRITE "${_external_include_dir}/zug/transducer/map.hpp" [=[
#pragma once

#include <utility>

#ifndef ZUG_FWD
#define ZUG_FWD(x) std::forward<decltype(x)>(x)
#endif

namespace zug {

template <typename Func>
struct map_result
{
    Func func;

    template <typename Reducer, typename Value>
    auto operator()(Reducer &&reducer, Value &&value) const
    {
        return reducer(func(std::forward<Value>(value)));
    }
};

template <typename Func>
map_result<Func> map(Func func)
{
    return {func};
}

} // namespace zug
]=])

file(WRITE "${_external_include_dir}/half.h" [=[
#pragma once

#include <cstdint>

class half
{
public:
    half() = default;
    half(float value)
        : m_value(value)
    {
    }

    operator float() const
    {
        return m_value;
    }

private:
    float m_value = 0.0f;
};
]=])

file(WRITE "${_external_include_dir}/halfLimits.h" [=[
#pragma once

#include "half.h"
]=])

file(WRITE "${_external_include_dir}/unwindstack/Regs.h" [=[
#pragma once

namespace unwindstack {

class Regs
{
public:
    static int CurrentArch()
    {
        return 0;
    }

    static Regs *CreateFromUcontext(int, void *)
    {
        return new Regs();
    }

    static Regs *CreateFromLocal()
    {
        return new Regs();
    }
};

} // namespace unwindstack
]=])

file(WRITE "${_external_include_dir}/unwindstack/Unwinder.h" [=[
#pragma once

#include <string>
#include <vector>
#include "Regs.h"

namespace unwindstack {

struct FrameData
{
};

class UnwinderFromPid
{
public:
    UnwinderFromPid(size_t, int, int)
    {
    }

    bool Init()
    {
        return true;
    }

    void SetRegs(Regs *)
    {
    }

    void Unwind()
    {
    }

    std::vector<FrameData> frames() const
    {
        return {};
    }

    const char *LastErrorCodeString() const
    {
        return "";
    }

    int LastErrorCode() const
    {
        return 0;
    }

    std::string FormatFrame(const FrameData &) const
    {
        return {};
    }
};

} // namespace unwindstack
]=])

include_directories("${_external_include_dir}")
set(unwindstack_INCLUDE_DIR "${_external_include_dir}")
set(unwindstack_LIBRARIES "")

add_library(KF5::I18n INTERFACE IMPORTED GLOBAL)
set_target_properties(KF5::I18n PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_external_include_dir}")

add_library(KF5::ConfigCore INTERFACE IMPORTED GLOBAL)
set_target_properties(KF5::ConfigCore PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_external_include_dir}")

add_library(Boost::boost INTERFACE IMPORTED GLOBAL)
set_target_properties(Boost::boost PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include")

add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include/eigen3")

add_library(lager INTERFACE)
target_include_directories(lager INTERFACE "${_external_include_dir}")

add_library(zug INTERFACE)
target_include_directories(zug INTERFACE "${_external_include_dir}")

set(HIDE_SAFE_ASSERTS ON)
set(CRASH_ON_SAFE_ASSERTS OFF)
set(GSL_FOUND OFF)
set(HAVE_GSL 0)
set(ANDROID_ENABLE_STDIO_FORWARDING OFF)
configure_file("${KRITA_SOURCE_DIR}/KoConfig.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/KoConfig.h")
configure_file("${KRITA_SOURCE_DIR}/config-gsl.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-gsl.h")
configure_file("${KRITA_SOURCE_DIR}/config-safe-asserts.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-safe-asserts.h")
configure_file("${KRITA_SOURCE_DIR}/config-android-stdio-forwarding.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-android-stdio-forwarding.h")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(_kritaversion_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaversion")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/version" "${_kritaversion_build_dir}")

target_include_directories(kritaversion
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/version>"
        "$<BUILD_INTERFACE:${_kritaversion_build_dir}>")

set(_kritaglobal_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaglobal")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/global" "${_kritaglobal_build_dir}")

target_include_directories(kritaglobal
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/global>"
        "$<BUILD_INTERFACE:${_kritaglobal_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")

set(_signal_mapper_moc "${_kritaglobal_build_dir}/moc_KisSignalMapper.cpp")
add_custom_command(
    OUTPUT "${_signal_mapper_moc}"
    COMMAND "${_qt5_moc_executable}"
        -DQT_CORE_LIB
        -DQT_GUI_LIB
        -DQT_WIDGETS_LIB
        -DQT_XML_LIB
        -DQT_ANDROIDEXTRAS_LIB
        -DQT_NO_DEBUG
        -Dkritaglobal_EXPORTS
        -I"${KRITA_SOURCE_DIR}/libs/global"
        -I"${_kritaglobal_build_dir}"
        -I"${CMAKE_CURRENT_BINARY_DIR}"
        -I"${_external_include_dir}"
        -I"${ANDROID_PREFIX}/include"
        -I"${ANDROID_PREFIX}/include/QtCore"
        -I"${ANDROID_PREFIX}/include/QtGui"
        -I"${ANDROID_PREFIX}/include/QtWidgets"
        -o "${_signal_mapper_moc}"
        "${KRITA_SOURCE_DIR}/libs/global/KisSignalMapper.h"
    DEPENDS
        "${KRITA_SOURCE_DIR}/libs/global/KisSignalMapper.h"
        "${_kritaglobal_build_dir}/kritaglobal_export.h"
    VERBATIM)
add_custom_target(kritaglobal_seed_included_moc DEPENDS "${_signal_mapper_moc}")
add_dependencies(kritaglobal kritaglobal_seed_included_moc)
