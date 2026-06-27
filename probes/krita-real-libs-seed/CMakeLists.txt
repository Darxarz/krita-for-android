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

macro(kis_add_test)
endmacro()

function(qt_add_resources out_var)
    set(_generated_resources "")

    foreach(_qrc IN LISTS ARGN)
        get_filename_component(_qrc_name "${_qrc}" NAME_WE)
        set(_generated "${CMAKE_CURRENT_BINARY_DIR}/qrc_${_qrc_name}_seed.cpp")
        file(WRITE "${_generated}" [=[
int qInitResources_]=] "${_qrc_name}" [=[()
{
    return 1;
}

int qCleanupResources_]=] "${_qrc_name}" [=[()
{
    return 1;
}
]=])
        list(APPEND _generated_resources "${_generated}")
    endforeach()

    set(${out_var} ${${out_var}} ${_generated_resources} PARENT_SCOPE)
endfunction()

function(ki18n_wrap_ui out_var)
    set(_generated_ui_headers "")

    foreach(_ui IN LISTS ARGN)
        get_filename_component(_ui_name "${_ui}" NAME_WE)
        set(_generated_header "${CMAKE_CURRENT_BINARY_DIR}/ui_${_ui_name}.h")

        if(_ui_name STREQUAL "kshortcutwidget")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include "kkeysequencewidget.h"

namespace Ui {

class KisKShortcutWidget
{
public:
    KisKKeySequenceWidget *priEditor = nullptr;
    KisKKeySequenceWidget *altEditor = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QVBoxLayout(parent);
        layout->addWidget(new QLabel(QStringLiteral("Primary"), parent));
        priEditor = new KisKKeySequenceWidget(parent);
        layout->addWidget(priEditor);
        layout->addWidget(new QLabel(QStringLiteral("Alternate"), parent));
        altEditor = new KisKKeySequenceWidget(parent);
        layout->addWidget(altEditor);
    }
};

} // namespace Ui
]=])
        else()
            file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${_ui}" _ui_content)
            string(REGEX MATCH "<class>([^<]+)</class>" _ui_class_match "${_ui_content}")
            set(_ui_class "${CMAKE_MATCH_1}")
            if(NOT _ui_class)
                set(_ui_class "${_ui_name}")
            endif()
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QWidget>

namespace Ui {

class ]=] "${_ui_class}" [=[
{
public:
    void setupUi(QWidget *)
    {
    }
};

} // namespace Ui
]=])
        endif()

        list(APPEND _generated_ui_headers "${_generated_header}")
    endforeach()

    set(${out_var} ${${out_var}} ${_generated_ui_headers} PARENT_SCOPE)
endfunction()

function(disable_seed_tests relative_dir)
    set(_tests_cmake "${KRITA_SOURCE_DIR}/${relative_dir}/tests/CMakeLists.txt")
    if(EXISTS "${_tests_cmake}")
        file(WRITE "${_tests_cmake}" "# Disabled by the Android real-libs seed probe.\n")
    endif()
endfunction()

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
add_android_qt5_module(Sql)
add_android_qt5_module(PrintSupport)
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
set(Qt5Core_VERSION_PATCH 2)
set(Qt5Core_MOC_EXECUTABLE "${_qt5_moc_executable}")
set_property(TARGET Qt5::Core PROPERTY INTERFACE_QT_MAJOR_VERSION 5)
set(Qt5Gui_PRIVATE_INCLUDE_DIRS
    "${ANDROID_PREFIX}/include/QtGui/${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}.${Qt5Core_VERSION_PATCH}"
    "${ANDROID_PREFIX}/include/QtGui/${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}.${Qt5Core_VERSION_PATCH}/QtGui")

set(_external_include_dir "${CMAKE_CURRENT_BINARY_DIR}/generated-externals")
file(MAKE_DIRECTORY
    "${_external_include_dir}"
    "${_external_include_dir}/lager"
    "${_external_include_dir}/gsl"
    "${_external_include_dir}/zug/compat"
    "${_external_include_dir}/zug/reducing"
    "${_external_include_dir}/zug/transducer"
    "${_external_include_dir}/unwindstack")

if(EXISTS "/usr/include/boost")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
        "/usr/include/boost" "${_external_include_dir}/boost")
endif()

if(EXISTS "/usr/include/xsimd")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
        "/usr/include/xsimd" "${_external_include_dir}/xsimd")
else()
    message(FATAL_ERROR "Missing xsimd headers from libxsimd-dev")
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
file(WRITE "${_external_include_dir}/KLocalizedString" [=[
#pragma once
#include "klocalizedstring.h"
]=])

file(WRITE "${_external_include_dir}/kconfig.h" [=[
#pragma once

#include <QMap>
#include <QString>

class KConfigGroup;

class KConfigBase
{
public:
    virtual ~KConfigBase() = default;

    KConfigGroup group(const QString &name) const;
    QString name() const
    {
        return QString();
    }

    void sync()
    {
    }
};

class KConfig : public KConfigBase
{
public:
    enum OpenFlag {
        SimpleConfig = 0x01
    };
    using OpenFlags = int;

    explicit KConfig(const QString &name = QString(), OpenFlags = 0)
        : m_name(name)
    {
    }

    QString name() const
    {
        return m_name;
    }

private:
    QString m_name;
};
]=])
file(WRITE "${_external_include_dir}/KConfig" [=[
#pragma once
#include "kconfig.h"
]=])

file(WRITE "${_external_include_dir}/ksharedconfig.h" [=[
#pragma once

#include <QSharedPointer>
#include <QString>
#include "kconfig.h"

class KConfigGroup;

class KSharedConfig : public KConfig
{
public:
    using Ptr = QSharedPointer<KSharedConfig>;

    explicit KSharedConfig(const QString &name = QString())
        : KConfig(name)
    {
    }

    static Ptr openConfig(const QString &name = QString())
    {
        return Ptr::create(name);
    }
};

using KSharedConfigPtr = KSharedConfig::Ptr;
]=])
file(WRITE "${_external_include_dir}/KSharedConfig" [=[
#pragma once
#include "ksharedconfig.h"
]=])

file(WRITE "${_external_include_dir}/kconfiggroup.h" [=[
#pragma once

#include <QMap>
#include <QStringList>
#include <QString>
#include <QSharedPointer>
#include "ksharedconfig.h"

class KConfigGroup
{
public:
    enum WriteConfigFlag {
        Normal = 0x00,
        Persistent = 0x01
    };
    using WriteConfigFlags = int;

    KConfigGroup() = default;

    KConfigGroup(const KSharedConfig::Ptr &config, const char *name)
        : m_config(config.data())
        , m_name(QString::fromUtf8(name ? name : ""))
    {
    }

    KConfigGroup(const KSharedConfig::Ptr &config, const QString &name)
        : m_config(config.data())
        , m_name(name)
    {
    }

    KConfigGroup(KConfigBase *config, const char *name)
        : m_config(config)
        , m_name(QString::fromUtf8(name ? name : ""))
    {
    }

    KConfigGroup(KConfigBase *config, const QString &name)
        : m_config(config)
        , m_name(name)
    {
    }

    KConfigGroup(const KConfigGroup *config, const char *name)
        : m_config(config ? config->config() : nullptr)
        , m_name(QString::fromUtf8(name ? name : ""))
    {
    }

    KConfigGroup(const KConfigGroup *config, const QString &name)
        : m_config(config ? config->config() : nullptr)
        , m_name(name)
    {
    }

    template <typename T>
    T readEntry(const QString &, const T &defaultValue = T()) const
    {
        return defaultValue;
    }

    QString readEntry(const QString &, const char *defaultValue) const
    {
        return QString::fromUtf8(defaultValue ? defaultValue : "");
    }

    QString readEntry(const QString &) const
    {
        return QString();
    }

    template <typename T>
    void writeEntry(const QString &, const T &, WriteConfigFlags = Normal)
    {
    }

    bool hasKey(const QString &) const
    {
        return false;
    }

    bool hasGroup(const QString &) const
    {
        return false;
    }

    QMap<QString, QString> entryMap() const
    {
        return {};
    }

    QStringList groupList() const
    {
        return {};
    }

    void deleteEntry(const QString &)
    {
    }

    void sync()
    {
    }

    QString name() const
    {
        return m_name;
    }

    KConfigBase *config() const
    {
        return m_config;
    }

private:
    KConfigBase *m_config = nullptr;
    QString m_name;
};

inline KConfigGroup KConfigBase::group(const QString &name) const
{
    return KConfigGroup(const_cast<KConfigBase *>(this), name);
}
]=])
file(WRITE "${_external_include_dir}/KConfigGroup" [=[
#pragma once
#include "kconfiggroup.h"
]=])

file(WRITE "${_external_include_dir}/kcolorutils.h" [=[
#pragma once

#include <algorithm>
#include <QColor>

namespace KColorUtils {

inline qreal clampUnit(qreal value)
{
    return std::max<qreal>(0.0, std::min<qreal>(1.0, value));
}

inline qreal luma(const QColor &color)
{
    return 0.2126 * color.redF() + 0.7152 * color.greenF() + 0.0722 * color.blueF();
}

inline QColor mix(const QColor &one, const QColor &two, qreal bias = 0.5)
{
    const qreal b = clampUnit(bias);
    return QColor::fromRgbF(
        one.redF() * (1.0 - b) + two.redF() * b,
        one.greenF() * (1.0 - b) + two.greenF() * b,
        one.blueF() * (1.0 - b) + two.blueF() * b,
        one.alphaF() * (1.0 - b) + two.alphaF() * b);
}

inline QColor tint(const QColor &base, const QColor &color, qreal amount = 0.5)
{
    return mix(base, color, amount);
}

inline QColor shade(const QColor &color, qreal amount, qreal = 0.0)
{
    return amount >= 0.0 ? mix(color, Qt::white, amount) : mix(color, Qt::black, -amount);
}

inline QColor lighten(const QColor &color, qreal amount)
{
    return shade(color, amount);
}

inline QColor darken(const QColor &color, qreal amount, qreal = 1.0)
{
    return shade(color, -amount);
}

} // namespace KColorUtils
]=])
file(WRITE "${_external_include_dir}/KColorUtils" [=[
#pragma once
#include "kcolorutils.h"
]=])

file(WRITE "${_external_include_dir}/kguiitem.h" [=[
#pragma once

#include <QIcon>
#include <QPushButton>
#include <QString>

class KGuiItem
{
public:
    explicit KGuiItem(const QString &text = QString(),
                      const QString &iconName = QString(),
                      const QString &toolTip = QString(),
                      const QString &whatsThis = QString())
        : m_text(text)
        , m_iconName(iconName)
        , m_toolTip(toolTip)
        , m_whatsThis(whatsThis)
    {
    }

    QString text() const
    {
        return m_text;
    }

    QIcon icon() const
    {
        return QIcon::fromTheme(m_iconName);
    }

    static void assign(QPushButton *button, const KGuiItem &item)
    {
        if (!button) {
            return;
        }

        button->setText(item.text());
        button->setIcon(item.icon());
        button->setToolTip(item.m_toolTip);
        button->setWhatsThis(item.m_whatsThis);
    }

private:
    QString m_text;
    QString m_iconName;
    QString m_toolTip;
    QString m_whatsThis;
};
]=])
file(WRITE "${_external_include_dir}/KGuiItem" [=[
#pragma once
#include "kguiitem.h"
]=])

file(WRITE "${_external_include_dir}/kstandardguiitem.h" [=[
#pragma once

#include "kguiitem.h"

namespace KStandardGuiItem {

inline KGuiItem ok()
{
    return KGuiItem(QStringLiteral("OK"));
}

inline KGuiItem cancel()
{
    return KGuiItem(QStringLiteral("Cancel"));
}

inline KGuiItem close()
{
    return KGuiItem(QStringLiteral("Close"));
}

inline KGuiItem apply()
{
    return KGuiItem(QStringLiteral("Apply"));
}

inline KGuiItem defaults()
{
    return KGuiItem(QStringLiteral("Defaults"));
}

inline KGuiItem del()
{
    return KGuiItem(QStringLiteral("Delete"));
}

inline KGuiItem save()
{
    return KGuiItem(QStringLiteral("Save"));
}

inline KGuiItem discard()
{
    return KGuiItem(QStringLiteral("Discard"));
}

inline KGuiItem print()
{
    return KGuiItem(QStringLiteral("Print"));
}

} // namespace KStandardGuiItem
]=])
file(WRITE "${_external_include_dir}/KStandardGuiItem" [=[
#pragma once
#include "kstandardguiitem.h"
]=])

file(WRITE "${_external_include_dir}/kmessagebox.h" [=[
#pragma once

#include <QString>
#include <QWidget>
#include "kguiitem.h"

namespace KMessageBox {

enum ButtonCode {
    Continue = 1,
    Cancel = 2,
    Yes = 3,
    No = 4,
    PrimaryAction = 5,
    SecondaryAction = 6
};

using Options = int;

inline ButtonCode warningContinueCancel(QWidget *, const QString &, const QString & = QString(),
                                        const KGuiItem & = KGuiItem(), const KGuiItem & = KGuiItem(),
                                        const QString & = QString(), Options = 0)
{
    return Continue;
}

inline ButtonCode warningYesNoCancel(QWidget *, const QString &, const QString & = QString(),
                                     const KGuiItem & = KGuiItem(), const KGuiItem & = KGuiItem(),
                                     const KGuiItem & = KGuiItem(), const QString & = QString(),
                                     Options = 0)
{
    return Yes;
}

inline ButtonCode questionTwoActions(QWidget *, const QString &, const QString & = QString(),
                                     const KGuiItem & = KGuiItem(), const KGuiItem & = KGuiItem(),
                                     const QString & = QString(), Options = 0)
{
    return PrimaryAction;
}

inline void error(QWidget *, const QString &, const QString & = QString())
{
}

} // namespace KMessageBox
]=])
file(WRITE "${_external_include_dir}/KMessageBox" [=[
#pragma once
#include "kmessagebox.h"
]=])

file(WRITE "${_external_include_dir}/ktitlewidget.h" [=[
#pragma once

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QWidget>

class KTitleWidget : public QWidget
{
public:
    enum ImageAlignment {
        ImageLeft,
        ImageRight
    };

    explicit KTitleWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_iconLabel(new QLabel(this))
        , m_textLabel(new QLabel(this))
    {
        auto *layout = new QHBoxLayout(this);
        layout->addWidget(m_iconLabel);
        layout->addWidget(m_textLabel);
    }

    void setText(const QString &text)
    {
        m_textLabel->setText(text);
    }

    void setIcon(const QIcon &icon, ImageAlignment = ImageLeft)
    {
        m_iconLabel->setPixmap(icon.pixmap(32));
    }

    void setPixmap(const QPixmap &pixmap, ImageAlignment = ImageLeft)
    {
        m_iconLabel->setPixmap(pixmap);
    }

private:
    QLabel *m_iconLabel = nullptr;
    QLabel *m_textLabel = nullptr;
};
]=])
file(WRITE "${_external_include_dir}/KTitleWidget" [=[
#pragma once
#include "ktitlewidget.h"
]=])

file(WRITE "${_external_include_dir}/kwidgetsaddons_version.h" [=[
#pragma once

#define KWIDGETSADDONS_VERSION_MAJOR 5
#define KWIDGETSADDONS_VERSION_MINOR 100
#define KWIDGETSADDONS_VERSION_PATCH 0
]=])

file(WRITE "${_external_include_dir}/kaboutdata.h" [=[
#pragma once

#include <QString>

class KAboutData
{
public:
    static KAboutData applicationData()
    {
        return KAboutData();
    }

    QString productName() const
    {
        return QStringLiteral("krita");
    }

    QString displayName() const
    {
        return QStringLiteral("Krita");
    }

    QString version() const
    {
        return QString();
    }

    QString bugAddress() const
    {
        return QString();
    }

    QString programIconName() const
    {
        return QStringLiteral("krita");
    }
};
]=])
file(WRITE "${_external_include_dir}/KAboutData" [=[
#pragma once
#include "kaboutdata.h"
]=])

file(WRITE "${_external_include_dir}/kemailsettings.h" [=[
#pragma once

#include <QString>

class KEMailSettings
{
public:
    enum Setting {
        RealName,
        EmailAddress
    };

    QString getSetting(Setting) const
    {
        return QString();
    }
};
]=])

file(WRITE "${_external_include_dir}/kwindowconfig.h" [=[
#pragma once

#include <QWindow>
#include "kconfiggroup.h"

class KWindowConfig
{
public:
    static void saveWindowSize(QWindow *, KConfigGroup &)
    {
    }

    static void saveWindowSize(QWindow *, const KConfigGroup &)
    {
    }

    static void restoreWindowSize(QWindow *, const KConfigGroup &)
    {
    }
};
]=])
file(WRITE "${_external_include_dir}/KWindowConfig" [=[
#pragma once
#include "kwindowconfig.h"
]=])

file(WRITE "${_external_include_dir}/kconfiggui.h" [=[
#pragma once

#include <QString>
#include "kconfig.h"

class KConfigGui
{
public:
    static void setSessionConfig(const QString &, const QString &)
    {
    }

    static KConfig *sessionConfig()
    {
        static KConfig config(QStringLiteral("krita-session"));
        return &config;
    }
};
]=])
file(WRITE "${_external_include_dir}/KConfigGui" [=[
#pragma once
#include "kconfiggui.h"
]=])

file(WRITE "${_external_include_dir}/kicontheme.h" [=[
#pragma once

class KIconTheme
{
};
]=])
file(WRITE "${_external_include_dir}/KIconTheme" [=[
#pragma once
#include "kicontheme.h"
]=])

file(WRITE "${_external_include_dir}/ktoggleaction.h" [=[
#pragma once

#include <QAction>
#include <QIcon>
#include <QString>

class KToggleAction : public QAction
{
public:
    explicit KToggleAction(QObject *parent = nullptr)
        : QAction(parent)
    {
        initialize();
    }

    explicit KToggleAction(const QString &text, QObject *parent = nullptr)
        : QAction(text, parent)
    {
        initialize();
    }

    KToggleAction(const QIcon &icon, const QString &text, QObject *parent = nullptr)
        : QAction(icon, text, parent)
    {
        initialize();
    }

protected:
    virtual void slotToggled(bool)
    {
    }

private:
    void initialize()
    {
        setCheckable(true);
        QObject::connect(this, &QAction::toggled, this, &KToggleAction::slotToggled);
    }
};
]=])
file(WRITE "${_external_include_dir}/KToggleAction" [=[
#pragma once
#include "ktoggleaction.h"
]=])

file(WRITE "${_external_include_dir}/kpluginfactory.h" [=[
#pragma once

#include <QObject>
#include <QVariantList>

class KPluginFactory : public QObject
{
    Q_OBJECT

public:
    explicit KPluginFactory(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    template <typename T>
    T *create(QObject *parent = nullptr, const QVariantList &args = QVariantList())
    {
        Q_UNUSED(args)
        return new T(parent);
    }
};
]=])
file(WRITE "${_external_include_dir}/KPluginFactory" [=[
#pragma once
#include "kpluginfactory.h"
]=])

file(WRITE "${_external_include_dir}/quazipnewinfo.h" [=[
#pragma once

#include <QFileDevice>
#include <QString>

class QuaZipNewInfo
{
public:
    explicit QuaZipNewInfo(const QString &name)
        : name(name)
    {
    }

    void setPermissions(QFileDevice::Permissions)
    {
    }

    QString name;
};
]=])

file(WRITE "${_external_include_dir}/quazip.h" [=[
#pragma once

#include <QIODevice>
#include <QString>
#include <QStringList>

#ifndef ZIP_OK
#define ZIP_OK 0
#endif

class QuaZip
{
public:
    enum Mode {
        mdCreate,
        mdUnzip
    };

    explicit QuaZip(const QString &)
    {
    }

    explicit QuaZip(QIODevice *device)
        : m_device(device)
    {
    }

    void setDataDescriptorWritingEnabled(bool)
    {
    }

    void setZip64Enabled(bool)
    {
    }

    void setFileNameCodec(const char *)
    {
    }

    QIODevice *getIoDevice() const
    {
        return m_device;
    }

    void setAutoClose(bool)
    {
    }

    bool open(Mode)
    {
        return true;
    }

    void close()
    {
    }

    int getZipError() const
    {
        return ZIP_OK;
    }

    int getEntriesCount() const
    {
        return 0;
    }

    QStringList getFileNameList() const
    {
        return QStringList();
    }

    bool setCurrentFile(const QString &)
    {
        return false;
    }

private:
    QIODevice *m_device = nullptr;
};
]=])

file(WRITE "${_external_include_dir}/quazipfile.h" [=[
#pragma once

#include <cstring>
#include <QByteArray>
#include <QIODevice>
#include "quazip.h"
#include "quazipnewinfo.h"

class QuaZipFile : public QIODevice
{
public:
    explicit QuaZipFile(QuaZip *, QObject *parent = nullptr)
        : QIODevice(parent)
    {
    }

    bool open(OpenMode mode) override
    {
        QIODevice::open(mode);
        return true;
    }

    bool open(OpenMode mode, const QuaZipNewInfo &, const char * = nullptr,
              unsigned int = 0, int = 0, int = 0)
    {
        QIODevice::open(mode);
        return true;
    }

    void close() override
    {
        QIODevice::close();
    }

    int getZipError() const
    {
        return ZIP_OK;
    }

    qint64 size() const override
    {
        return m_buffer.size();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 available = qMin(maxSize, qint64(m_buffer.size() - m_pos));
        if (available <= 0) {
            return -1;
        }
        memcpy(data, m_buffer.constData() + m_pos, size_t(available));
        m_pos += available;
        return available;
    }

    qint64 writeData(const char *data, qint64 maxSize) override
    {
        m_buffer.append(data, int(maxSize));
        return maxSize;
    }

private:
    QByteArray m_buffer;
    qint64 m_pos = 0;
};
]=])

file(WRITE "${_external_include_dir}/quazipdir.h" [=[
#pragma once

#include <QString>
#include "quazip.h"

class QuaZipDir
{
public:
    QuaZipDir(QuaZip *, const QString &)
    {
    }

    bool exists() const
    {
        return true;
    }
};
]=])

file(WRITE "${_external_include_dir}/quazipfileinfo.h" [=[
#pragma once

class QuaZipFileInfo
{
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

file(WRITE "${_external_include_dir}/gsl/gsl_multimin.h" [=[
#pragma once

#include <cstddef>

struct gsl_vector
{
};

inline double gsl_vector_get(const gsl_vector *, size_t)
{
    return 0.0;
}

inline void gsl_vector_set(gsl_vector *, size_t, double)
{
}
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

add_library(KF5::CoreAddons INTERFACE IMPORTED GLOBAL)
set_target_properties(KF5::CoreAddons PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_external_include_dir}")

foreach(_kf_target IN ITEMS ItemViews ConfigGui GuiAddons WidgetsAddons)
    add_library(KF5::${_kf_target} INTERFACE IMPORTED GLOBAL)
    set_target_properties(KF5::${_kf_target} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_external_include_dir}")
endforeach()

add_library(Boost::boost INTERFACE IMPORTED GLOBAL)
set_target_properties(Boost::boost PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include")

add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include/eigen3")

add_library(xsimd INTERFACE)
target_include_directories(xsimd INTERFACE "${_external_include_dir}")

add_library(lager INTERFACE)
target_include_directories(lager INTERFACE "${_external_include_dir}")

add_library(zug INTERFACE)
target_include_directories(zug INTERFACE "${_external_include_dir}")

set(HIDE_SAFE_ASSERTS ON)
set(CRASH_ON_SAFE_ASSERTS OFF)
set(HAVE_DBUS OFF)
set(GSL_FOUND OFF)
set(HAVE_GSL 0)
set(ANDROID_ENABLE_STDIO_FORWARDING OFF)
set(QT_QTCORE_LIBRARY Qt5::Core)
set(QUAZIP_INCLUDE_DIRS "${_external_include_dir}")
set(QUAZIP_LIBRARIES "")
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
target_compile_definitions(kritaglobal PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritaglobal PRIVATE
    -include "${_external_include_dir}/gsl/gsl_multimin.h")

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
        -DQT_NO_SIGNALS_SLOTS_KEYWORDS
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

disable_seed_tests(libs/koplugin)

set(_kritaplugin_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaplugin")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/koplugin" "${_kritaplugin_build_dir}")

target_sources(kritaplugin PRIVATE
    "${_external_include_dir}/kpluginfactory.h")
target_include_directories(kritaplugin
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/koplugin>"
        "$<BUILD_INTERFACE:${_kritaplugin_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaplugin PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

set(_kritamultiarch_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritamultiarch")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/multiarch" "${_kritamultiarch_build_dir}")

target_include_directories(kritamultiarch
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/multiarch>"
        "$<BUILD_INTERFACE:${_kritamultiarch_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritamultiarch PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

set(_kritacolor_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritacolor")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/color" "${_kritacolor_build_dir}")

target_include_directories(kritacolor
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/color>"
        "$<BUILD_INTERFACE:${_kritacolor_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritacolor PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

disable_seed_tests(libs/store)

set(_kritastore_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritastore")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/store" "${_kritastore_build_dir}")

target_include_directories(kritastore
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/store>"
        "$<BUILD_INTERFACE:${_kritastore_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritastore PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

disable_seed_tests(libs/resources)

set(_kritaresources_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaresources")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/resources" "${_kritaresources_build_dir}")

target_include_directories(kritaresources
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/resources>"
        "$<BUILD_INTERFACE:${_kritaresources_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaresources PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

disable_seed_tests(libs/widgetutils)

set(_kritawidgetutils_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritawidgetutils")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/widgetutils" "${_kritawidgetutils_build_dir}")

target_include_directories(kritawidgetutils
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/widgetutils>"
        "$<BUILD_INTERFACE:${_kritawidgetutils_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritawidgetutils PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

disable_seed_tests(libs/command)

set(_kritacommand_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritacommand")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/command" "${_kritacommand_build_dir}")

target_include_directories(kritacommand
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/command>"
        "$<BUILD_INTERFACE:${_kritacommand_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritacommand PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
