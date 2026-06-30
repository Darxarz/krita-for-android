cmake_minimum_required(VERSION 3.21)
project(krita_real_libs_seed LANGUAGES C CXX)

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
set(HAVE_HDR OFF)
set(HAVE_OCIO OFF)

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

function(disable_seed_subdir relative_dir)
    set(_subdir_cmake "${KRITA_SOURCE_DIR}/${relative_dir}/CMakeLists.txt")
    if(EXISTS "${_subdir_cmake}")
        file(WRITE "${_subdir_cmake}" "# Disabled by the Android real-libs seed probe.\n")
    endif()
endfunction()

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

        if(_ui_name STREQUAL "KisShortcutsDialog")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QGuiApplication>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QStringList>
#include <ktreewidgetsearchlinewidget.h>

namespace Ui {

class KisShortcutsDialog
{
public:
    KTreeWidgetSearchLineWidget *searchFilter = nullptr;
    QTreeWidget *list = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QVBoxLayout(parent);
        searchFilter = new KTreeWidgetSearchLineWidget(parent);
        list = new QTreeWidget(parent);
        list->setColumnCount(3);
        list->setHeaderLabels(QStringList()
                              << QStringLiteral("Action")
                              << QStringLiteral("Shortcut")
                              << QStringLiteral("Alternate"));
        list->setRootIsDecorated(true);
        list->setSortingEnabled(true);
        layout->addWidget(searchFilter);
        layout->addWidget(list);
    }
};

} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "PathToolOptionWidgetBase")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace Ui {

class PathToolOptionWidgetBase
{
public:
    QToolButton *corner = nullptr;
    QToolButton *smooth = nullptr;
    QToolButton *symmetric = nullptr;
    QToolButton *lineSegment = nullptr;
    QToolButton *curveSegment = nullptr;
    QToolButton *linePoint = nullptr;
    QToolButton *curvePoint = nullptr;
    QToolButton *addPoint = nullptr;
    QToolButton *removePoint = nullptr;
    QToolButton *breakPoint = nullptr;
    QToolButton *breakSegment = nullptr;
    QToolButton *joinSegment = nullptr;
    QToolButton *mergePoints = nullptr;
    QToolButton *convertToPath = nullptr;
    QStackedWidget *stackedWidget = nullptr;
    QWidget *wdgShapeProperties = nullptr;
    QWidget *lineShapeProperties = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QVBoxLayout(parent);
        corner = makeToolButton(parent, layout);
        smooth = makeToolButton(parent, layout);
        symmetric = makeToolButton(parent, layout);
        lineSegment = makeToolButton(parent, layout);
        curveSegment = makeToolButton(parent, layout);
        linePoint = makeToolButton(parent, layout);
        curvePoint = makeToolButton(parent, layout);
        addPoint = makeToolButton(parent, layout);
        removePoint = makeToolButton(parent, layout);
        breakPoint = makeToolButton(parent, layout);
        breakSegment = makeToolButton(parent, layout);
        joinSegment = makeToolButton(parent, layout);
        mergePoints = makeToolButton(parent, layout);
        convertToPath = makeToolButton(parent, layout);

        stackedWidget = new QStackedWidget(parent);
        stackedWidget->addWidget(new QWidget(stackedWidget));
        stackedWidget->addWidget(new QWidget(stackedWidget));
        layout->addWidget(stackedWidget);

        wdgShapeProperties = new QWidget(parent);
        wdgShapeProperties->setLayout(new QVBoxLayout(wdgShapeProperties));
        lineShapeProperties = new QWidget(parent);
        lineShapeProperties->setLayout(new QVBoxLayout(lineShapeProperties));
        layout->addWidget(wdgShapeProperties);
        layout->addWidget(lineShapeProperties);
    }

private:
    QToolButton *makeToolButton(QWidget *parent, QVBoxLayout *layout)
    {
        auto *button = new QToolButton(parent);
        layout->addWidget(button);
        return button;
    }
};

} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "KoZoomToolWidget")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QGridLayout>
#include <QRadioButton>
#include <QWidget>

namespace Ui {

class ZoomToolWidget
{
public:
    QRadioButton *zoomInButton = nullptr;
    QRadioButton *zoomOutButton = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QGridLayout(parent);
        zoomInButton = new QRadioButton(QStringLiteral("&Zoom In"), parent);
        zoomOutButton = new QRadioButton(QStringLiteral("Zoo&m Out"), parent);
        zoomInButton->setFocusPolicy(Qt::NoFocus);
        zoomOutButton->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(zoomInButton, 0, 0);
        layout->addWidget(zoomOutButton, 0, 1);
        layout->setRowStretch(1, 1);
    }
};

} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "KoConfigAuthorPage")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QString>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace Ui {

class KoConfigAuthorPage
{
public:
    QLineEdit *leNickName = nullptr;
    QLineEdit *leFirstName = nullptr;
    QLineEdit *leInitials = nullptr;
    QLineEdit *leLastName = nullptr;
    QLineEdit *leTitle = nullptr;
    QLineEdit *lePosition = nullptr;
    QLineEdit *leCompany = nullptr;
    QTableView *tblContactInfo = nullptr;
    QToolButton *btnAdd = nullptr;
    QToolButton *btnRemove = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QVBoxLayout(parent);
        auto *form = new QFormLayout();
        layout->addLayout(form);

        leNickName = addLineEdit(parent, form, QStringLiteral("Nickname"));
        leFirstName = addLineEdit(parent, form, QStringLiteral("First name"));
        leInitials = addLineEdit(parent, form, QStringLiteral("Initials"));
        leLastName = addLineEdit(parent, form, QStringLiteral("Last name"));
        leTitle = addLineEdit(parent, form, QStringLiteral("Title"));
        lePosition = addLineEdit(parent, form, QStringLiteral("Position"));
        leCompany = addLineEdit(parent, form, QStringLiteral("Company"));

        tblContactInfo = new QTableView(parent);
        layout->addWidget(tblContactInfo);

        auto *buttons = new QHBoxLayout();
        btnAdd = new QToolButton(parent);
        btnRemove = new QToolButton(parent);
        buttons->addWidget(btnAdd);
        buttons->addWidget(btnRemove);
        layout->addLayout(buttons);
    }

private:
    QLineEdit *addLineEdit(QWidget *parent, QFormLayout *form, const QString &label)
    {
        auto *lineEdit = new QLineEdit(parent);
        form->addRow(label, lineEdit);
        return lineEdit;
    }
};

} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "wdggeneralsettings")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "KisColorSamplerPreviewPreview.h"
#include "kis_color_button.h"
#include "kis_file_name_requester.h"
#include "kis_font_family_combo_box.h"
#include "kis_int_parse_spin_box.h"
#include "kis_slider_spin_box.h"
#include "kis_warning_block.h"

namespace Ui {

class WdgGeneralSettings
{
public:
    QTabWidget *tabWidget = nullptr;
    QWidget *File = nullptr;
    QGroupBox *m_autosaveCheckBox = nullptr;
    KisIntParseSpinBox *m_autosaveSpinBox = nullptr;
    QCheckBox *chkHideAutosaveFiles = nullptr;
    QGroupBox *m_backupFileCheckBox = nullptr;
    QComboBox *cmbBackupFileLocation = nullptr;
    QLineEdit *txtBackupFileSuffix = nullptr;
    QSpinBox *intNumBackupFiles = nullptr;
    QCheckBox *chkZip64 = nullptr;
    QCheckBox *m_chkCompressKra = nullptr;
    QCheckBox *m_chkTrimKra = nullptr;
    QCheckBox *m_chkTrimFramesImport = nullptr;
    QComboBox *cmbDefaultExportFileType = nullptr;
    QWidget *Pasting = nullptr;
    QRadioButton *btnDownload = nullptr;
    QRadioButton *btnLocal = nullptr;
    QRadioButton *btnBitmap = nullptr;
    QRadioButton *btnAsk = nullptr;
    QWidget *Window = nullptr;
    QFormLayout *formLayout = nullptr;
    QComboBox *m_cmbMDIType = nullptr;
    QCheckBox *m_chkCanvasMessages = nullptr;
    KisIntParseSpinBox *intZoomMarginSize = nullptr;
    QGroupBox *group_subWinMode = nullptr;
    QWidget *widget = nullptr;
    QLabel *m_backgroundimage = nullptr;
    QToolButton *m_bnFileName = nullptr;
    QPushButton *clearBgImageButton = nullptr;
    QLabel *label_5 = nullptr;
    KisColorButton *m_mdiColor = nullptr;
    QCheckBox *m_chkRubberBand = nullptr;
    QGroupBox *chkUseCustomFont = nullptr;
    KisFontComboBoxes *cmbCustomFont = nullptr;
    QSpinBox *intFontSize = nullptr;
    QGroupBox *m_chkHiDPI = nullptr;
    QWidget *m_wdgHiDPIFractionalScaling = nullptr;
    QCheckBox *m_chkHiDPIFractionalScaling = nullptr;
    QWidget *Cursor = nullptr;
    QComboBox *m_cmbCursorShape = nullptr;
    QComboBox *m_cmbOutlineShape = nullptr;
    QGroupBox *groupBox = nullptr;
    QCheckBox *m_showOutlinePainting = nullptr;
    QCheckBox *m_changeBrushOutline = nullptr;
    KisColorButton *cursorColorButton = nullptr;
    QGroupBox *m_chkSeparateEraserCursor = nullptr;
    QComboBox *m_cmbEraserCursorShape = nullptr;
    QComboBox *m_cmbEraserOutlineShape = nullptr;
    QCheckBox *m_showEraserOutlinePainting = nullptr;
    QCheckBox *m_changeEraserBrushOutline = nullptr;
    KisColorButton *eraserCursorColorButton = nullptr;
    QComboBox *m_cmbColorSamplerPreviewStyle = nullptr;
    QFrame *m_frmColorSamplerCircleSettings = nullptr;
    KisSliderSpinBox *m_nmbColorSamplerPreviewSize = nullptr;
    KisColorSamplerPreviewPreview *m_lblColorSamplerPreviewSizePreview = nullptr;
    KisDoubleSliderSpinBox *m_ssbColorSamplerPreviewThickness = nullptr;
    QCheckBox *m_chkColorSamplerPreviewOutlineEnabled = nullptr;
    QCheckBox *m_chkColorSamplerPreviewExtraCircles = nullptr;
    QWidget *Tools = nullptr;
    QRadioButton *m_radioToolOptionsInDocker = nullptr;
    QComboBox *cmbFlowMode = nullptr;
    QComboBox *cmbCmykBlendingMode = nullptr;
    QCheckBox *m_chkSwitchSelectionCtrlAlt = nullptr;
    QComboBox *cmbTouchPainting = nullptr;
    QCheckBox *chkTouchPressureSensitivity = nullptr;
    QCheckBox *chkEnableTransformToolAfterPaste = nullptr;
    QSpinBox *spnZoomSteps = nullptr;
    QCheckBox *chkZoomHorizontally = nullptr;
    QCheckBox *chkEnableSelectionActionBar = nullptr;
    QWidget *Animation = nullptr;
    QCheckBox *m_chkAutoPin = nullptr;
    QCheckBox *m_chkAdaptivePlaybackRange = nullptr;
    QCheckBox *m_chkAutoZoom = nullptr;
    QWidget *Resources = nullptr;
    KisFileNameRequester *m_urlResourceFolder = nullptr;
    QComboBox *m_resourceFolderSelector = nullptr;
    KisWarningBlock *grpWindowsAppData = nullptr;
    KisWarningBlock *grpNonWritableLocation = nullptr;
    KisWarningBlock *grpRestartMessage = nullptr;
    KisWarningBlock *grpAndroidWarningMessage = nullptr;
    QWidget *Miscellaneous = nullptr;
    QWidget *scrollAreaWidgetContents = nullptr;
    QFormLayout *formLayout_13 = nullptr;
    QComboBox *cmbStartupSession = nullptr;
    QCheckBox *chkSaveSessionOnQuit = nullptr;
    QCheckBox *m_chkConvertOnImport = nullptr;
    QLabel *label = nullptr;
    KisIntParseSpinBox *m_undoStackSize = nullptr;
    QCheckBox *chkCumulativeUndo = nullptr;
    QPushButton *btnAdvancedCumulativeUndo = nullptr;
    QCheckBox *chkShowRootLayer = nullptr;
    QCheckBox *chkUsageLogging = nullptr;
    QCheckBox *m_chkNativeFileDialog = nullptr;
    QSpinBox *intMaxBrushSize = nullptr;
    QGroupBox *chkForcedFontDPI = nullptr;
    QSpinBox *intForcedFontDPI = nullptr;
    QCheckBox *chkRenameMergedLayers = nullptr;
    QCheckBox *chkRenamePastedLayers = nullptr;
    QCheckBox *chkRenameDuplicatedLayers = nullptr;
    QCheckBox *chkEnableLongPress = nullptr;
    QGroupBox *m_groupBoxKineticScrollingSettings = nullptr;
    QComboBox *m_cmbKineticScrollingGesture = nullptr;
    KisSliderSpinBox *m_kineticScrollingSensitivitySlider = nullptr;
    QCheckBox *m_chkKineticScrollingHideScrollbars = nullptr;
    QCheckBox *chkIgnoreHighFunctionKeys = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *rootLayout = new QVBoxLayout(parent);
        tabWidget = makeWidget<QTabWidget>(parent);
        rootLayout->addWidget(tabWidget);

        File = makeTab(QStringLiteral("File Handling"));
        Pasting = makeTab(QStringLiteral("Pasting"));
        Window = makeTab(QStringLiteral("Window"));
        Cursor = makeTab(QStringLiteral("Cursor"));
        Tools = makeTab(QStringLiteral("Tools"));
        Animation = makeTab(QStringLiteral("Animation"));
        Resources = makeTab(QStringLiteral("Resources"));
        Miscellaneous = makeTab(QStringLiteral("Miscellaneous"));

        m_autosaveCheckBox = makeWidget<QGroupBox>(parent);
        m_autosaveSpinBox = makeWidget<KisIntParseSpinBox>(parent);
        chkHideAutosaveFiles = makeWidget<QCheckBox>(parent);
        m_backupFileCheckBox = makeWidget<QGroupBox>(parent);
        cmbBackupFileLocation = makeWidget<QComboBox>(parent);
        txtBackupFileSuffix = makeWidget<QLineEdit>(parent);
        intNumBackupFiles = makeWidget<QSpinBox>(parent);
        chkZip64 = makeWidget<QCheckBox>(parent);
        m_chkCompressKra = makeWidget<QCheckBox>(parent);
        m_chkTrimKra = makeWidget<QCheckBox>(parent);
        m_chkTrimFramesImport = makeWidget<QCheckBox>(parent);
        cmbDefaultExportFileType = makeWidget<QComboBox>(parent);
        btnDownload = makeWidget<QRadioButton>(parent);
        btnLocal = makeWidget<QRadioButton>(parent);
        btnBitmap = makeWidget<QRadioButton>(parent);
        btnAsk = makeWidget<QRadioButton>(parent);
        formLayout = new QFormLayout();
        m_cmbMDIType = makeWidget<QComboBox>(parent);
        m_chkCanvasMessages = makeWidget<QCheckBox>(parent);
        intZoomMarginSize = makeWidget<KisIntParseSpinBox>(parent);
        group_subWinMode = makeWidget<QGroupBox>(parent);
        widget = makeWidget<QWidget>(parent);
        m_backgroundimage = makeWidget<QLabel>(parent);
        m_bnFileName = makeWidget<QToolButton>(parent);
        clearBgImageButton = makeWidget<QPushButton>(parent);
        label_5 = makeWidget<QLabel>(parent);
        m_mdiColor = makeWidget<KisColorButton>(parent);
        m_chkRubberBand = makeWidget<QCheckBox>(parent);
        chkUseCustomFont = makeWidget<QGroupBox>(parent);
        cmbCustomFont = makeWidget<KisFontComboBoxes>(parent);
        auto *stylesComboBox = new QComboBox(cmbCustomFont);
        stylesComboBox->setObjectName(QStringLiteral("stylesComboBox"));
        intFontSize = makeWidget<QSpinBox>(parent);
        m_chkHiDPI = makeWidget<QGroupBox>(parent);
        m_wdgHiDPIFractionalScaling = makeWidget<QWidget>(parent);
        m_chkHiDPIFractionalScaling = makeWidget<QCheckBox>(parent);
        m_cmbCursorShape = makeWidget<QComboBox>(parent);
        m_cmbOutlineShape = makeWidget<QComboBox>(parent);
        groupBox = makeWidget<QGroupBox>(parent);
        m_showOutlinePainting = makeWidget<QCheckBox>(parent);
        m_changeBrushOutline = makeWidget<QCheckBox>(parent);
        cursorColorButton = makeWidget<KisColorButton>(parent);
        m_chkSeparateEraserCursor = makeWidget<QGroupBox>(parent);
        m_cmbEraserCursorShape = makeWidget<QComboBox>(parent);
        m_cmbEraserOutlineShape = makeWidget<QComboBox>(parent);
        m_showEraserOutlinePainting = makeWidget<QCheckBox>(parent);
        m_changeEraserBrushOutline = makeWidget<QCheckBox>(parent);
        eraserCursorColorButton = makeWidget<KisColorButton>(parent);
        m_cmbColorSamplerPreviewStyle = makeWidget<QComboBox>(parent);
        m_frmColorSamplerCircleSettings = makeWidget<QFrame>(parent);
        m_nmbColorSamplerPreviewSize = makeWidget<KisSliderSpinBox>(parent);
        m_lblColorSamplerPreviewSizePreview = makeWidget<KisColorSamplerPreviewPreview>(parent);
        m_ssbColorSamplerPreviewThickness = makeWidget<KisDoubleSliderSpinBox>(parent);
        m_chkColorSamplerPreviewOutlineEnabled = makeWidget<QCheckBox>(parent);
        m_chkColorSamplerPreviewExtraCircles = makeWidget<QCheckBox>(parent);
        m_radioToolOptionsInDocker = makeWidget<QRadioButton>(parent);
        cmbFlowMode = makeWidget<QComboBox>(parent);
        cmbCmykBlendingMode = makeWidget<QComboBox>(parent);
        m_chkSwitchSelectionCtrlAlt = makeWidget<QCheckBox>(parent);
        cmbTouchPainting = makeWidget<QComboBox>(parent);
        chkTouchPressureSensitivity = makeWidget<QCheckBox>(parent);
        chkEnableTransformToolAfterPaste = makeWidget<QCheckBox>(parent);
        spnZoomSteps = makeWidget<QSpinBox>(parent);
        chkZoomHorizontally = makeWidget<QCheckBox>(parent);
        chkEnableSelectionActionBar = makeWidget<QCheckBox>(parent);
        m_chkAutoPin = makeWidget<QCheckBox>(parent);
        m_chkAdaptivePlaybackRange = makeWidget<QCheckBox>(parent);
        m_chkAutoZoom = makeWidget<QCheckBox>(parent);
        m_urlResourceFolder = makeWidget<KisFileNameRequester>(parent);
        m_resourceFolderSelector = makeWidget<QComboBox>(parent);
        grpWindowsAppData = makeWidget<KisWarningBlock>(parent);
        grpNonWritableLocation = makeWidget<KisWarningBlock>(parent);
        grpRestartMessage = makeWidget<KisWarningBlock>(parent);
        grpAndroidWarningMessage = makeWidget<KisWarningBlock>(parent);
        scrollAreaWidgetContents = makeWidget<QWidget>(parent);
        formLayout_13 = new QFormLayout(scrollAreaWidgetContents);
        cmbStartupSession = makeWidget<QComboBox>(parent);
        chkSaveSessionOnQuit = makeWidget<QCheckBox>(parent);
        m_chkConvertOnImport = makeWidget<QCheckBox>(parent);
        label = makeWidget<QLabel>(parent);
        m_undoStackSize = makeWidget<KisIntParseSpinBox>(parent);
        chkCumulativeUndo = makeWidget<QCheckBox>(parent);
        btnAdvancedCumulativeUndo = makeWidget<QPushButton>(parent);
        chkShowRootLayer = makeWidget<QCheckBox>(scrollAreaWidgetContents);
        formLayout_13->addRow(QString(), chkShowRootLayer);
        chkUsageLogging = makeWidget<QCheckBox>(parent);
        m_chkNativeFileDialog = makeWidget<QCheckBox>(parent);
        intMaxBrushSize = makeWidget<QSpinBox>(parent);
        chkForcedFontDPI = makeWidget<QGroupBox>(parent);
        intForcedFontDPI = makeWidget<QSpinBox>(parent);
        chkRenameMergedLayers = makeWidget<QCheckBox>(parent);
        chkRenamePastedLayers = makeWidget<QCheckBox>(parent);
        chkRenameDuplicatedLayers = makeWidget<QCheckBox>(parent);
        chkEnableLongPress = makeWidget<QCheckBox>(parent);
        m_groupBoxKineticScrollingSettings = makeWidget<QGroupBox>(parent);
        m_cmbKineticScrollingGesture = makeWidget<QComboBox>(parent);
        m_kineticScrollingSensitivitySlider = makeWidget<KisSliderSpinBox>(parent);
        m_chkKineticScrollingHideScrollbars = makeWidget<QCheckBox>(parent);
        chkIgnoreHighFunctionKeys = makeWidget<QCheckBox>(parent);
    }

private:
    template <typename Widget>
    Widget *makeWidget(QWidget *parent)
    {
        return new Widget(parent);
    }

    QWidget *makeTab(const QString &title)
    {
        auto *tab = new QWidget(tabWidget);
        tabWidget->addTab(tab, title);
        return tab;
    }
};

} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "wdgpreseticonlibrary")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QListView>
#include <QWidget>

#include "kis_slider_spin_box.h"

class Ui_wdgpreseticonlibrary
{
public:
    KisDoubleSliderSpinBox *sldHue = nullptr;
    KisDoubleSliderSpinBox *sldSat = nullptr;
    KisDoubleSliderSpinBox *sldLevels = nullptr;
    QListView *vwBase = nullptr;
    QListView *vwOptional = nullptr;
    QLabel *lblIconPreview = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QGridLayout(parent);
        sldHue = new KisDoubleSliderSpinBox(parent);
        sldSat = new KisDoubleSliderSpinBox(parent);
        sldLevels = new KisDoubleSliderSpinBox(parent);
        vwBase = new QListView(parent);
        vwOptional = new QListView(parent);
        lblIconPreview = new QLabel(parent);

        layout->addWidget(sldHue, 0, 0);
        layout->addWidget(sldSat, 1, 0);
        layout->addWidget(sldLevels, 2, 0);
        layout->addWidget(vwBase, 0, 1);
        layout->addWidget(vwOptional, 1, 1);
        layout->addWidget(lblIconPreview, 2, 1);
    }
};

namespace Ui {
class wdgpreseticonlibrary : public ::Ui_wdgpreseticonlibrary
{
};
} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "wdg_file_name_requester")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QWidget>

namespace Ui {

class WdgFileNameRequester
{
public:
    QLineEdit *txtFileName = nullptr;
    QToolButton *btnSelectFile = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QHBoxLayout(parent);
        txtFileName = new QLineEdit(parent);
        btnSelectFile = new QToolButton(parent);
        layout->addWidget(txtFileName);
        layout->addWidget(btnSelectFile);
    }
};

} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "WdgDlgInternalColorSelector")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "KisPaletteComboBox.h"
#include "KisPopupButton.h"
#include "KisSpinboxHSXSelector.h"
#include "KisVisualColorSelector.h"
#include "KoColorPatch.h"
#include "kis_palette_view.h"
#include "kis_spinbox_color_selector.h"

class Ui_WdgDlgInternalColorSelector
{
public:
    KisVisualColorSelector *visualSelector = nullptr;
    QTabWidget *tabWidget = nullptr;
    QWidget *tab_channel = nullptr;
    KisSpinboxColorSelector *spinboxselector = nullptr;
    QWidget *tab_hsx = nullptr;
    KisSpinboxHSXSelector *spinboxHSXSelector = nullptr;
    KoColorPatch *currentColor = nullptr;
    KoColorPatch *previousColor = nullptr;
    QVBoxLayout *rightPane = nullptr;
    KisPaletteView *paletteBox = nullptr;
    KisPopupButton *bnPaletteChooser = nullptr;
    KisPaletteComboBox *cmbNameList = nullptr;
    QWidget *screenColorSamplerWidget = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

    void setupUi(QDialog *parent)
    {
        setupUi(static_cast<QWidget *>(parent));
    }

    void setupUi(QWidget *parent)
    {
        auto *verticalLayout = new QVBoxLayout(parent);
        auto *horizontal = new QHBoxLayout();
        verticalLayout->addLayout(horizontal);

        auto *leftPane = new QVBoxLayout();
        horizontal->addLayout(leftPane);

        visualSelector = new KisVisualColorSelector(parent);
        visualSelector->setMinimumSize(90, 90);
        leftPane->addWidget(visualSelector);

        tabWidget = new QTabWidget(parent);
        tabWidget->setTabBarAutoHide(true);

        tab_channel = new QWidget(tabWidget);
        auto *channelLayout = new QVBoxLayout(tab_channel);
        channelLayout->setContentsMargins(0, 0, 0, 0);
        channelLayout->setSpacing(0);
        spinboxselector = new KisSpinboxColorSelector(tab_channel);
        channelLayout->addWidget(spinboxselector);
        tabWidget->addTab(tab_channel, QStringLiteral("Channels"));

        tab_hsx = new QWidget(tabWidget);
        auto *hsxLayout = new QVBoxLayout(tab_hsx);
        hsxLayout->setContentsMargins(0, 0, 0, 0);
        hsxLayout->setSpacing(0);
        spinboxHSXSelector = new KisSpinboxHSXSelector(tab_hsx);
        hsxLayout->addWidget(spinboxHSXSelector);
        tabWidget->addTab(tab_hsx, QStringLiteral("Unknown"));

        leftPane->addWidget(tabWidget);

        auto *frame = new QFrame(parent);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Sunken);
        frame->setLineWidth(2);
        frame->setMidLineWidth(2);
        auto *nextPrevLayout = new QHBoxLayout(frame);
        nextPrevLayout->setContentsMargins(0, 0, 0, 0);
        nextPrevLayout->setSpacing(0);
        currentColor = new KoColorPatch(frame);
        currentColor->setMinimumSize(50, 70);
        previousColor = new KoColorPatch(frame);
        previousColor->setMinimumSize(25, 25);
        nextPrevLayout->addWidget(currentColor);
        nextPrevLayout->addWidget(previousColor);
        leftPane->addWidget(frame);

        rightPane = new QVBoxLayout();
        horizontal->addLayout(rightPane);

        paletteBox = new KisPaletteView(parent);
        paletteBox->setMinimumWidth(90);
        rightPane->addWidget(paletteBox);

        auto *paletteRow = new QHBoxLayout();
        paletteRow->setContentsMargins(0, 0, 0, 0);
        rightPane->addLayout(paletteRow);
        bnPaletteChooser = new KisPopupButton(parent);
        bnPaletteChooser->setMinimumWidth(50);
        cmbNameList = new KisPaletteComboBox(parent);
        cmbNameList->setEditable(true);
        paletteRow->addWidget(bnPaletteChooser);
        paletteRow->addWidget(cmbNameList);

        screenColorSamplerWidget = new QWidget(parent);
        screenColorSamplerWidget->setMinimumSize(50, 50);
        rightPane->addWidget(screenColorSamplerWidget);

        buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, parent);
        buttonBox->setOrientation(Qt::Horizontal);
        verticalLayout->addWidget(buttonBox);
    }
};

namespace Ui {
class WdgDlgInternalColorSelector : public ::Ui_WdgDlgInternalColorSelector
{
};
} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "WdgPaletteListWidget")
            file(WRITE "${_generated_header}" [=[
#pragma once

#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class Ui_WdgPaletteListWidget
{
public:
    QWidget *viewPalette = nullptr;
    QToolButton *bnAdd = nullptr;
    QToolButton *bnRemove = nullptr;
    QToolButton *bnImport = nullptr;
    QToolButton *bnExport = nullptr;

    void setupUi(QWidget *parent)
    {
        auto *layout = new QVBoxLayout(parent);
        viewPalette = new QWidget(parent);
        layout->addWidget(viewPalette);

        auto *ctrlLine = new QWidget(parent);
        auto *buttonLayout = new QHBoxLayout(ctrlLine);
        buttonLayout->addStretch();

        bnAdd = new QToolButton(ctrlLine);
        bnExport = new QToolButton(ctrlLine);
        bnImport = new QToolButton(ctrlLine);
        bnRemove = new QToolButton(ctrlLine);

        buttonLayout->addWidget(bnAdd);
        buttonLayout->addWidget(bnExport);
        buttonLayout->addWidget(bnImport);
        buttonLayout->addWidget(bnRemove);
        layout->addWidget(ctrlLine);
    }
};

namespace Ui {
class WdgPaletteListWidget : public ::Ui_WdgPaletteListWidget
{
};
} // namespace Ui
]=])
        elseif(_ui_name STREQUAL "kshortcutwidget")
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
            if(_qt5_uic_executable)
                execute_process(
                    COMMAND "${_qt5_uic_executable}"
                        -o "${_generated_header}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/${_ui}"
                    RESULT_VARIABLE _seed_uic_result
                    OUTPUT_VARIABLE _seed_uic_stdout
                    ERROR_VARIABLE _seed_uic_stderr)
                if(NOT _seed_uic_result EQUAL 0)
                    message(FATAL_ERROR "uic failed for ${_ui}: ${_seed_uic_stderr}")
                endif()
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

using Ui_]=] "${_ui_class}" [=[ = Ui::]=] "${_ui_class}" [=[;
]=])
            endif()
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
add_android_qt5_module(Network)
add_android_qt5_module(Widgets)
add_android_qt5_module(Xml)
add_android_qt5_module(Svg)
add_android_qt5_module(Concurrent)
add_android_qt5_module(Sql)
add_android_qt5_module(PrintSupport)
add_android_qt5_module(AndroidExtras)

target_link_libraries(Qt5::Gui INTERFACE Qt5::Core)
target_link_libraries(Qt5::Network INTERFACE Qt5::Core)
target_link_libraries(Qt5::Widgets INTERFACE Qt5::Gui)
target_link_libraries(Qt5::Xml INTERFACE Qt5::Core)
target_link_libraries(Qt5::Svg INTERFACE Qt5::Widgets)
target_link_libraries(Qt5::Concurrent INTERFACE Qt5::Core)
target_link_libraries(Qt5::Sql INTERFACE Qt5::Core)
target_link_libraries(Qt5::PrintSupport INTERFACE Qt5::Widgets)
target_link_libraries(Qt5::AndroidExtras INTERFACE Qt5::Core)

find_program(_qt5_moc_executable
    NAMES moc
    HINTS "${ANDROID_PREFIX}/bin"
    NO_DEFAULT_PATH)
find_program(_qt5_uic_executable
    NAMES uic
    HINTS "${ANDROID_PREFIX}/bin"
    NO_DEFAULT_PATH)

if(NOT _qt5_moc_executable)
    message(FATAL_ERROR "Could not find host Qt moc under ${ANDROID_PREFIX}/bin")
endif()
if(NOT _qt5_uic_executable)
    message(FATAL_ERROR "Could not find host Qt uic under ${ANDROID_PREFIX}/bin")
endif()

if(NOT TARGET Qt5::moc)
    add_executable(Qt5::moc IMPORTED GLOBAL)
    set_target_properties(Qt5::moc PROPERTIES
        IMPORTED_LOCATION "${_qt5_moc_executable}")
endif()
if(NOT TARGET Qt5::uic)
    add_executable(Qt5::uic IMPORTED GLOBAL)
    set_target_properties(Qt5::uic PROPERTIES
        IMPORTED_LOCATION "${_qt5_uic_executable}")
endif()

set(Qt5Core_VERSION_MAJOR 5)
set(Qt5Core_VERSION_MINOR 15)
set(Qt5Core_VERSION_PATCH 2)
set(Qt5Core_MOC_EXECUTABLE "${_qt5_moc_executable}")
set_property(TARGET Qt5::Core PROPERTY INTERFACE_QT_MAJOR_VERSION 5)
set(Qt5Core_PRIVATE_INCLUDE_DIRS
    "${ANDROID_PREFIX}/include/QtCore/${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}.${Qt5Core_VERSION_PATCH}"
    "${ANDROID_PREFIX}/include/QtCore/${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}.${Qt5Core_VERSION_PATCH}/QtCore")
set(Qt5Gui_PRIVATE_INCLUDE_DIRS
    "${ANDROID_PREFIX}/include/QtGui/${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}.${Qt5Core_VERSION_PATCH}"
    "${ANDROID_PREFIX}/include/QtGui/${Qt5Core_VERSION_MAJOR}.${Qt5Core_VERSION_MINOR}.${Qt5Core_VERSION_PATCH}/QtGui"
    ${Qt5Core_PRIVATE_INCLUDE_DIRS})

add_compile_definitions(QT_USE_QSTRINGBUILDER)

set(_external_include_dir "${CMAKE_CURRENT_BINARY_DIR}/generated-externals")
file(MAKE_DIRECTORY
    "${_external_include_dir}"
    "${_external_include_dir}/lager"
    "${_external_include_dir}/lager/extra"
    "${_external_include_dir}/lager/lenses"
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

if(EXISTS "/usr/include/fontconfig")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
        "/usr/include/fontconfig" "${_external_include_dir}/fontconfig")
else()
    message(FATAL_ERROR "Missing fontconfig headers from libfontconfig1-dev")
endif()

file(GLOB _seed_unibreak_headers
    "/usr/include/*break*.h"
    "/usr/include/unibreak*.h")
foreach(_seed_unibreak_header IN LISTS _seed_unibreak_headers)
    get_filename_component(_seed_unibreak_header_name "${_seed_unibreak_header}" NAME)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${_seed_unibreak_header}"
        "${_external_include_dir}/${_seed_unibreak_header_name}")
endforeach()
foreach(_seed_required_unibreak_header IN ITEMS
        graphemebreak.h
        linebreak.h
        wordbreak.h)
    if(NOT EXISTS "${_external_include_dir}/${_seed_required_unibreak_header}")
        message(FATAL_ERROR "Missing ${_seed_required_unibreak_header} from libunibreak-dev")
    endif()
endforeach()

file(WRITE "${_external_include_dir}/klocalizedstring.h" [=[
#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>

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

    static QByteArray applicationDomain()
    {
        return QByteArray();
    }

    static bool isApplicationTranslatedInto(const QString &)
    {
        return false;
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
inline QString i18nd(const char *, const char *text, Args &&...)
{
    return QString::fromUtf8(text ? text : "");
}

template <typename... Args>
inline QString i18ndc(const char *, const char *, const char *text, Args &&...)
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

file(WRITE "${_external_include_dir}/kformat.h" [=[
#pragma once

#include <QString>

class KFormat
{
public:
    enum BinarySizeDialect {
        DefaultBinaryDialect,
        IECBinaryDialect,
        JEDECBinaryDialect,
        MetricBinaryDialect
    };

    enum BinaryUnitDialect {
        UnitByte,
        UnitKiloByte,
        UnitMegaByte,
        UnitGigaByte,
        UnitTeraByte,
        UnitPetaByte,
        UnitLastUnit
    };

    QString formatByteSize(double size,
                           int precision = 1,
                           BinarySizeDialect = DefaultBinaryDialect,
                           BinaryUnitDialect units = UnitByte) const
    {
        static const char *suffixes[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
        double value = size;
        int unit = static_cast<int>(units);
        if (unit <= static_cast<int>(UnitByte)) {
            unit = 0;
            while (value >= 1024.0 && unit < static_cast<int>(UnitPetaByte)) {
                value /= 1024.0;
                ++unit;
            }
        } else {
            while (unit > 0) {
                value /= 1024.0;
                --unit;
            }
            unit = static_cast<int>(units);
        }

        return QStringLiteral("%1 %2").arg(value, 0, 'f', precision).arg(QString::fromLatin1(suffixes[unit]));
    }
};
]=])
file(WRITE "${_external_include_dir}/KFormat" [=[
#pragma once
#include "kformat.h"
]=])

file(WRITE "${_external_include_dir}/seed_kritawidgets_forced_includes.h" [=[
#pragma once

#include <QSet>
#include "klocalizedstring.h"
]=])

file(WRITE "${_external_include_dir}/seed_kritaui_forced_includes.h" [=[
#pragma once

#include <QIcon>
#include <QItemSelection>
#include <QLabel>
#include <QDialogButtonBox>
#include <QToolButton>
#include "klocalizedstring.h"
]=])

file(WRITE "${_external_include_dir}/kcombobox.h" [=[
#pragma once

#include <QComboBox>

class KComboBox : public QComboBox
{
public:
    explicit KComboBox(QWidget *parent = nullptr)
        : QComboBox(parent)
    {
    }
};
]=])
file(WRITE "${_external_include_dir}/KComboBox" [=[
#pragma once
#include "kcombobox.h"
]=])

file(WRITE "${_external_include_dir}/kcolorbutton.h" [=[
#pragma once

#include <QColor>
#include <QPushButton>

class KColorButton : public QPushButton
{
public:
    explicit KColorButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
    }

    QColor color() const
    {
        return m_color;
    }

    QColor defaultColor() const
    {
        return m_defaultColor;
    }

    void setColor(const QColor &color)
    {
        m_color = color;
    }

    void setDefaultColor(const QColor &color)
    {
        m_defaultColor = color;
    }

    void setAlphaChannelEnabled(bool enabled)
    {
        m_alphaChannelEnabled = enabled;
    }

    bool isAlphaChannelEnabled() const
    {
        return m_alphaChannelEnabled;
    }

    void changed(const QColor &)
    {
    }

private:
    QColor m_color;
    QColor m_defaultColor;
    bool m_alphaChannelEnabled = false;
};
]=])
file(WRITE "${_external_include_dir}/KColorButton" [=[
#pragma once
#include "kcolorbutton.h"
]=])

file(WRITE "${_external_include_dir}/kcolormimedata.h" [=[
#pragma once

#include <QColor>
#include <QDrag>
#include <QMimeData>
#include <QVariant>
#include <QWidget>

class KColorMimeData
{
public:
    static bool canDecode(const QMimeData *mimeData)
    {
        return mimeData && mimeData->hasColor();
    }

    static QColor fromMimeData(const QMimeData *mimeData)
    {
        return canDecode(mimeData) ? mimeData->colorData().value<QColor>() : QColor();
    }

    static QDrag *createDrag(const QColor &color, QWidget *source)
    {
        auto *drag = new QDrag(source);
        auto *mimeData = new QMimeData;
        mimeData->setColorData(color);
        drag->setMimeData(mimeData);
        return drag;
    }
};
]=])

file(WRITE "${_external_include_dir}/png.h" [=[
#pragma once

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PNG_LIBPNG_VER 10600
#define PNG_LIBPNG_VER_MAJOR 1
#define PNG_LIBPNG_VER_MINOR 6
#define PNG_LIBPNG_VER_STRING "1.6.0"
#define PNG_UINT_31_MAX 0x7fffffffU

#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_RGB 2
#define PNG_COLOR_TYPE_PALETTE 3
#define PNG_COLOR_TYPE_GRAY_ALPHA 4
#define PNG_COLOR_TYPE_RGB_ALPHA 6
#define PNG_INTERLACE_NONE 0
#define PNG_INTERLACE_ADAM7 1
#define PNG_COMPRESSION_TYPE_DEFAULT 0
#define PNG_COMPRESSION_TYPE_BASE 0
#define PNG_FILTER_TYPE_DEFAULT 0
#define PNG_TEXT_COMPRESSION_zTXt 0
#define PNG_INFO_tRNS 0x0010U
#define PNG_RESOLUTION_UNKNOWN 0
#define PNG_RESOLUTION_METER 1
#define PNG_OPTION_ON 1
#define PNG_TRANSFORM_IDENTITY 0
#define PNG_sRGB_INTENT_PERCEPTUAL 0

typedef uint32_t png_uint_32;
typedef size_t png_size_t;
typedef unsigned char png_byte;
typedef png_byte *png_bytep;
typedef png_byte **png_bytepp;
typedef const png_byte *png_const_bytep;
typedef char png_char;
typedef png_char *png_charp;
typedef png_char **png_charpp;
typedef const png_char *png_const_charp;

typedef struct png_color_struct {
    png_byte red;
    png_byte green;
    png_byte blue;
} png_color;
typedef png_color *png_colorp;

typedef struct png_text_struct {
    int compression;
    png_charp key;
    png_charp text;
    png_size_t text_length;
} png_text;
typedef png_text *png_textp;

typedef struct png_info_def {
    int unused;
} png_info;
typedef png_info *png_infop;
typedef png_info **png_infopp;

typedef struct png_struct_def {
    jmp_buf jmpbuf;
    void *io_ptr;
} png_struct;
typedef png_struct *png_structp;

#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)

typedef void (*png_error_ptr)(png_structp, png_const_charp);
typedef void (*png_rw_ptr)(png_structp, png_bytep, png_size_t);
typedef void (*png_flush_ptr)(png_structp);

int png_check_sig(png_bytep sig, int num);
int png_sig_cmp(png_bytep sig, png_size_t start, png_size_t num_to_check);
png_structp png_create_read_struct(png_const_charp user_png_ver, void *error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
png_structp png_create_write_struct(png_const_charp user_png_ver, void *error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn);
png_infop png_create_info_struct(png_structp png_ptr);
void png_destroy_read_struct(png_structp *png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr);
void png_destroy_write_struct(png_structp *png_ptr_ptr, png_infopp info_ptr_ptr);
void png_set_error_fn(png_structp png_ptr, void *error_ptr, png_error_ptr error_fn, png_error_ptr warning_fn);
void png_set_read_fn(png_structp png_ptr, void *io_ptr, png_rw_ptr read_data_fn);
void png_set_write_fn(png_structp png_ptr, void *io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn);
void *png_get_io_ptr(png_structp png_ptr);
void png_error(png_structp png_ptr, png_const_charp message);
void *png_malloc(png_structp png_ptr, png_uint_32 size);
void png_free(png_structp png_ptr, void *ptr);
void png_read_info(png_structp png_ptr, png_infop info_ptr);
void png_read_update_info(png_structp png_ptr, png_infop info_ptr);
void png_read_row(png_structp png_ptr, png_bytep row, png_bytep display_row);
void png_read_image(png_structp png_ptr, png_bytepp image);
void png_read_end(png_structp png_ptr, png_infop end_info);
void png_write_info(png_structp png_ptr, png_infop info_ptr);
void png_write_flush(png_structp png_ptr);
void png_write_image(png_structp png_ptr, png_bytepp image);
void png_write_end(png_structp png_ptr, png_infop info_ptr);
void png_set_expand(png_structp png_ptr);
void png_set_packing(png_structp png_ptr);
void png_set_swap(png_structp png_ptr);
void png_set_compression_level(png_structp png_ptr, int level);
void png_set_compression_mem_level(png_structp png_ptr, int mem_level);
void png_set_compression_strategy(png_structp png_ptr, int strategy);
void png_set_compression_window_bits(png_structp png_ptr, int window_bits);
void png_set_compression_method(png_structp png_ptr, int method);
void png_set_compression_buffer_size(png_structp png_ptr, png_size_t size);
png_size_t png_get_rowbytes(png_structp png_ptr, png_infop info_ptr);
int png_get_color_type(png_structp png_ptr, png_infop info_ptr);
int png_get_bit_depth(png_structp png_ptr, png_infop info_ptr);
png_uint_32 png_get_valid(png_structp png_ptr, png_infop info_ptr, png_uint_32 flag);
png_uint_32 png_get_IHDR(png_structp png_ptr, png_infop info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_type, int *compression_type, int *filter_method);
int png_get_iCCP(png_structp png_ptr, png_infop info_ptr, png_charpp name, int *compression_type, png_bytepp profile, png_uint_32 *proflen);
int png_get_text(png_structp png_ptr, png_infop info_ptr, png_textp *text_ptr, int *num_text);
int png_get_pHYs(png_structp png_ptr, png_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type);
int png_get_PLTE(png_structp png_ptr, png_infop info_ptr, png_colorp *palette, int *num_palette);
int png_get_tRNS(png_structp png_ptr, png_infop info_ptr, png_bytep *trans_alpha, int *num_trans, void *trans_color);
void png_set_IHDR(png_structp png_ptr, png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type, int compression_type, int filter_method);
void png_set_PLTE(png_structp png_ptr, png_infop info_ptr, png_colorp palette, int num_palette);
void png_set_text(png_structp png_ptr, png_infop info_ptr, png_textp text_ptr, int num_text);
void png_set_iCCP(png_structp png_ptr, png_infop info_ptr, png_const_charp name, int compression_type, png_const_bytep profile, png_uint_32 proflen);
void png_set_pHYs(png_structp png_ptr, png_infop info_ptr, png_uint_32 res_x, png_uint_32 res_y, int unit_type);
void png_set_sRGB_gAMA_and_cHRM(png_structp png_ptr, png_infop info_ptr, int intent);

#ifdef __cplusplus
}
#endif
]=])

file(WRITE "${_external_include_dir}/kconfig.h" [=[
#pragma once

#include <QMap>
#include <QStandardPaths>
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

    QMap<QString, QString> entryMap(const QString & = QString()) const
    {
        return {};
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
    T readPathEntry(const QString &, const T &defaultValue = T()) const
    {
        return defaultValue;
    }

    QString readPathEntry(const QString &, const char *defaultValue) const
    {
        return QString::fromUtf8(defaultValue ? defaultValue : "");
    }

    template <typename T>
    void writeEntry(const QString &, const T &, WriteConfigFlags = Normal)
    {
    }

    template <typename T>
    void writePathEntry(const QString &, const T &, WriteConfigFlags = Normal)
    {
    }

    bool hasKey(const QString &) const
    {
        return false;
    }

    bool exists() const
    {
        return false;
    }

    bool isValid() const
    {
        return true;
    }

    bool hasGroup(const QString &) const
    {
        return false;
    }

    KConfigGroup group(const QString &name) const
    {
        return KConfigGroup(config(), name);
    }

    KConfigGroup group(const char *name) const
    {
        return KConfigGroup(config(), name);
    }

    bool hasDefault(const QString &) const
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

    void deleteEntry(const QString &, WriteConfigFlags)
    {
    }

    void deleteGroup()
    {
    }

    void revertToDefault(const QString &)
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

inline KGuiItem help()
{
    return KGuiItem(QStringLiteral("Help"));
}

inline KGuiItem reset()
{
    return KGuiItem(QStringLiteral("Reset"));
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

#include <QMessageBox>
#include <QString>
#include <QWidget>
#include "kguiitem.h"
#include "kstandardguiitem.h"

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

file(WRITE "${_external_include_dir}/khelpclient.h" [=[
#pragma once

#include <QString>

class KHelpClient
{
public:
    static void invokeHelp(const QString & = QString(), const QString & = QString())
    {
    }
};
]=])
file(WRITE "${_external_include_dir}/KHelpClient" [=[
#pragma once
#include "khelpclient.h"
]=])

file(WRITE "${_external_include_dir}/kurllabel.h" [=[
#pragma once

#include <QFont>
#include <QLabel>
#include <QMouseEvent>
#include <QString>
#include <Qt>

class KUrlLabel : public QLabel
{
public:
    explicit KUrlLabel(QWidget *parent = nullptr)
        : QLabel(parent)
    {
    }

    explicit KUrlLabel(const QString &url, const QString &text = QString(), QWidget *parent = nullptr)
        : QLabel(text.isEmpty() ? url : text, parent)
        , m_url(url)
    {
    }

    void setUrl(const QString &url)
    {
        m_url = url;
    }

    QString url() const
    {
        return m_url;
    }

    void setFloatEnabled(bool)
    {
    }

    void setUnderline(bool underline)
    {
        QFont currentFont = font();
        currentFont.setUnderline(underline);
        setFont(currentFont);
    }

    void leftClickedUrl()
    {
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (!event || event->button() == Qt::LeftButton) {
            leftClickedUrl();
        }

        QLabel::mouseReleaseEvent(event);
    }

private:
    QString m_url;
};
]=])
file(WRITE "${_external_include_dir}/KUrlLabel" [=[
#pragma once
#include "kurllabel.h"
]=])

file(WRITE "${_external_include_dir}/kselector.h" [=[
#pragma once

#include <QPainter>
#include <QPaintEvent>
#include <QPoint>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

class KSelector : public QWidget
{
public:
    explicit KSelector(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }

    explicit KSelector(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_orientation(orientation)
    {
    }

    void setMinimum(int value)
    {
        m_minimum = value;
        if (m_value < m_minimum) {
            m_value = m_minimum;
        }
        update();
    }

    void setMaximum(int value)
    {
        m_maximum = value;
        if (m_value > m_maximum) {
            m_value = m_maximum;
        }
        update();
    }

    void setRange(int minimum, int maximum)
    {
        m_minimum = minimum;
        m_maximum = maximum;
        m_value = qBound(m_minimum, m_value, m_maximum);
        update();
    }

    int minimum() const
    {
        return m_minimum;
    }

    int maximum() const
    {
        return m_maximum;
    }

    int value() const
    {
        return m_value;
    }

    void setValue(int value)
    {
        m_value = qBound(m_minimum, value, m_maximum);
        update();
    }

    Qt::Orientation orientation() const
    {
        return m_orientation;
    }

    void setOrientation(Qt::Orientation orientation)
    {
        m_orientation = orientation;
        update();
    }

    Qt::ArrowType arrowDirection() const
    {
        if (m_arrowDirection != Qt::NoArrow) {
            return m_arrowDirection;
        }

        return m_orientation == Qt::Horizontal ? Qt::DownArrow : Qt::RightArrow;
    }

    void setArrowDirection(Qt::ArrowType direction)
    {
        m_arrowDirection = direction;
        update();
    }

protected:
    virtual void drawContents(QPainter *)
    {
    }

    virtual void drawArrow(QPainter *, const QPoint &)
    {
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        drawContents(&painter);
        drawArrow(&painter, rect().center());
    }

private:
    Qt::Orientation m_orientation = Qt::Horizontal;
    Qt::ArrowType m_arrowDirection = Qt::NoArrow;
    int m_minimum = 0;
    int m_maximum = 100;
    int m_value = 0;
};
]=])
file(WRITE "${_external_include_dir}/KSelector" [=[
#pragma once
#include "kselector.h"
]=])

file(WRITE "${_external_include_dir}/kpagedialog.h" [=[
#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QIcon>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QWidget>

class KPageWidgetItem : public QObject
{
public:
    explicit KPageWidgetItem(QWidget *widget, const QString &name = QString())
        : QObject(widget)
        , m_widget(widget)
        , m_name(name)
    {
    }

    QWidget *widget() const
    {
        return m_widget;
    }

    QString name() const
    {
        return m_name;
    }

    void setHeader(const QString &header)
    {
        m_header = header;
    }

    void setIcon(const QIcon &icon)
    {
        m_icon = icon;
    }

private:
    QWidget *m_widget = nullptr;
    QString m_name;
    QString m_header;
    QIcon m_icon;
};

class KPageDialog : public QDialog
{
public:
    enum FaceType {
        Auto,
        Plain,
        List,
        Tree,
        Tabbed
    };

    enum DialogCode {
        Rejected = QDialog::Rejected,
        Accepted = QDialog::Accepted
    };

    explicit KPageDialog(QWidget *parent = nullptr)
        : QDialog(parent)
        , m_buttonBox(new QDialogButtonBox(this))
    {
    }

    void setFaceType(FaceType)
    {
    }

    KPageWidgetItem *addPage(QWidget *widget, const QString &name)
    {
        m_currentPage = new KPageWidgetItem(widget, name);
        return m_currentPage;
    }

    void addPage(KPageWidgetItem *item)
    {
        if (item) {
            m_currentPage = item;
        }
    }

    KPageWidgetItem *currentPage() const
    {
        return m_currentPage;
    }

    void setCurrentPage(KPageWidgetItem *item)
    {
        if (item) {
            m_currentPage = item;
        }
    }

    void setStandardButtons(QDialogButtonBox::StandardButtons buttons)
    {
        m_buttonBox->setStandardButtons(buttons);
    }

    QPushButton *button(QDialogButtonBox::StandardButton which) const
    {
        return m_buttonBox->button(which);
    }

    QDialogButtonBox *buttonBox() const
    {
        return m_buttonBox;
    }

private:
    KPageWidgetItem *m_currentPage = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};
]=])
file(WRITE "${_external_include_dir}/KPageDialog" [=[
#pragma once
#include "kpagedialog.h"
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

file(WRITE "${_external_include_dir}/ksqueezedtextlabel.h" [=[
#pragma once

#include <QLabel>
#include <QString>
#include <Qt>

class KSqueezedTextLabel : public QLabel
{
public:
    explicit KSqueezedTextLabel(QWidget *parent = nullptr)
        : QLabel(parent)
    {
    }

    explicit KSqueezedTextLabel(const QString &text, QWidget *parent = nullptr)
        : QLabel(text, parent)
    {
    }

    void setTextElideMode(Qt::TextElideMode mode)
    {
        m_elideMode = mode;
    }

    Qt::TextElideMode textElideMode() const
    {
        return m_elideMode;
    }

private:
    Qt::TextElideMode m_elideMode = Qt::ElideMiddle;
};
]=])
file(WRITE "${_external_include_dir}/KSqueezedTextLabel" [=[
#pragma once
#include "ksqueezedtextlabel.h"
]=])

file(WRITE "${_external_include_dir}/kwidgetsaddons_version.h" [=[
#pragma once

#define KWIDGETSADDONS_VERSION_MAJOR 5
#define KWIDGETSADDONS_VERSION_MINOR 100
#define KWIDGETSADDONS_VERSION_PATCH 0
]=])

file(WRITE "${_external_include_dir}/config-seexpr.h" [=[
#pragma once

#define HAVE_SEEXPR 0
]=])

file(WRITE "${_external_include_dir}/kaboutdata.h" [=[
#pragma once

#include <QList>
#include <QString>

class KAboutPerson
{
public:
    explicit KAboutPerson(const QString &name = QString())
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

    QList<KAboutPerson> translators() const
    {
        return m_translators;
    }

    void setTranslator(const QString &name, const QString &)
    {
        m_translators = {KAboutPerson(name)};
    }

private:
    QList<KAboutPerson> m_translators;
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

file(WRITE "${_external_include_dir}/kuser.h" [=[
#pragma once

#include <QString>

class KUser
{
public:
    KUser()
    {
    }

    QString loginName() const
    {
        return QString();
    }

    QString fullName() const
    {
        return QString();
    }
};
]=])
file(WRITE "${_external_include_dir}/KUser" [=[
#pragma once
#include "kuser.h"
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

file(WRITE "${_external_include_dir}/kdualaction.h" [=[
#pragma once

#include <QIcon>
#include <QString>
#include <ktoggleaction.h>

class KDualAction : public KToggleAction
{
public:
    explicit KDualAction(QObject *parent = nullptr)
        : KToggleAction(parent)
    {
    }

    KDualAction(const QString &inactiveText, const QString &activeText, QObject *parent = nullptr)
        : KToggleAction(inactiveText, parent)
        , m_inactiveText(inactiveText)
        , m_activeText(activeText)
    {
    }

    void setActive(bool active)
    {
        setChecked(active);
        updateState(active);
    }

    bool isActive() const
    {
        return isChecked();
    }

    void setInactiveText(const QString &text)
    {
        m_inactiveText = text;
        updateState(isChecked());
    }

    void setActiveText(const QString &text)
    {
        m_activeText = text;
        updateState(isChecked());
    }

    void setInactiveIcon(const QIcon &icon)
    {
        m_inactiveIcon = icon;
        updateState(isChecked());
    }

    void setActiveIcon(const QIcon &icon)
    {
        m_activeIcon = icon;
        updateState(isChecked());
    }

private:
    void updateState(bool active)
    {
        if (active) {
            if (!m_activeText.isEmpty()) {
                setText(m_activeText);
            }
            if (!m_activeIcon.isNull()) {
                setIcon(m_activeIcon);
            }
        } else {
            if (!m_inactiveText.isEmpty()) {
                setText(m_inactiveText);
            }
            if (!m_inactiveIcon.isNull()) {
                setIcon(m_inactiveIcon);
            }
        }
    }

    QString m_inactiveText;
    QString m_activeText;
    QIcon m_inactiveIcon;
    QIcon m_activeIcon;
};
]=])
file(WRITE "${_external_include_dir}/KDualAction" [=[
#pragma once
#include "kdualaction.h"
]=])

file(WRITE "${_external_include_dir}/ktogglefullscreenaction.h" [=[
#pragma once

#include <QWidget>
#include <ktoggleaction.h>

class KToggleFullScreenAction : public KToggleAction
{
public:
    explicit KToggleFullScreenAction(QObject *parent = nullptr)
        : KToggleAction(parent)
    {
    }

    void setWindow(QWidget *window)
    {
        m_window = window;
    }

    QWidget *window() const
    {
        return m_window;
    }

protected:
    void slotToggled(bool checked) override
    {
        if (!m_window) {
            return;
        }
        if (checked) {
            m_window->showFullScreen();
        } else {
            m_window->showNormal();
        }
    }

private:
    QWidget *m_window = nullptr;
};
]=])
file(WRITE "${_external_include_dir}/KToggleFullScreenAction" [=[
#pragma once
#include "ktogglefullscreenaction.h"
]=])

file(WRITE "${_external_include_dir}/kextendableitemdelegate.h" [=[
#pragma once

#include <QList>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QWidget>

class KExtendableItemDelegate : public QStyledItemDelegate
{
public:
    enum {
        ShowExtensionIndicatorRole = Qt::UserRole + 1000
    };

    explicit KExtendableItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void setExtendPixmap(const QPixmap &pixmap)
    {
        m_extendPixmap = pixmap;
    }

    void setContractPixmap(const QPixmap &pixmap)
    {
        m_contractPixmap = pixmap;
    }

    bool isExtended(const QModelIndex &index) const
    {
        return m_extendedIndexes.contains(QPersistentModelIndex(index));
    }

    void extendItem(QWidget *widget, const QModelIndex &index)
    {
        if (widget) {
            widget->setParent(parentWidget());
            widget->hide();
        }
        const QPersistentModelIndex persistent(index);
        if (!m_extendedIndexes.contains(persistent)) {
            m_extendedIndexes.append(persistent);
        }
    }

    void contractItem(const QModelIndex &index)
    {
        m_extendedIndexes.removeAll(QPersistentModelIndex(index));
    }

    void contractAll()
    {
        m_extendedIndexes.clear();
    }

private:
    QWidget *parentWidget() const
    {
        return qobject_cast<QWidget *>(parent());
    }

    QPixmap m_extendPixmap;
    QPixmap m_contractPixmap;
    QList<QPersistentModelIndex> m_extendedIndexes;
};
]=])
file(WRITE "${_external_include_dir}/KExtendableItemDelegate" [=[
#pragma once
#include "kextendableitemdelegate.h"
]=])

file(WRITE "${_external_include_dir}/ktreewidgetsearchline.h" [=[
#pragma once

#include <QGuiApplication>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class KTreeWidgetSearchLine : public QLineEdit
{
public:
    explicit KTreeWidgetSearchLine(QWidget *parent = nullptr, QTreeWidget *treeWidget = nullptr)
        : QLineEdit(parent)
        , m_treeWidget(treeWidget)
    {
    }

    void setTreeWidget(QTreeWidget *treeWidget)
    {
        m_treeWidget = treeWidget;
    }

    QTreeWidget *treeWidget() const
    {
        return m_treeWidget;
    }

private:
    QTreeWidget *m_treeWidget = nullptr;
};
]=])

file(WRITE "${_external_include_dir}/ktreewidgetsearchlinewidget.h" [=[
#pragma once

#include <QVBoxLayout>
#include <QWidget>
#include <ktreewidgetsearchline.h>

class KTreeWidgetSearchLineWidget : public QWidget
{
public:
    explicit KTreeWidgetSearchLineWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_searchLine(new KTreeWidgetSearchLine(this))
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_searchLine);
    }

    KTreeWidgetSearchLine *searchLine() const
    {
        return m_searchLine;
    }

private:
    KTreeWidgetSearchLine *m_searchLine = nullptr;
};
]=])

file(WRITE "${_external_include_dir}/klistwidgetsearchline.h" [=[
#pragma once

#include <QLineEdit>
#include <QListWidget>

class KListWidgetSearchLine : public QLineEdit
{
public:
    explicit KListWidgetSearchLine(QWidget *parent = nullptr, QListWidget *listWidget = nullptr)
        : QLineEdit(parent)
        , m_listWidget(listWidget)
    {
    }

    void setListWidget(QListWidget *listWidget)
    {
        m_listWidget = listWidget;
    }

    QListWidget *listWidget() const
    {
        return m_listWidget;
    }

private:
    QListWidget *m_listWidget = nullptr;
};
]=])

file(WRITE "${_external_include_dir}/kseparator.h" [=[
#pragma once

#include <QFrame>

class KSeparator : public QFrame
{
public:
    explicit KSeparator(QWidget *parent = nullptr)
        : QFrame(parent)
    {
        applyOrientation(Qt::Horizontal);
        setFrameShadow(QFrame::Sunken);
    }

    explicit KSeparator(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QFrame(parent)
    {
        applyOrientation(orientation);
        setFrameShadow(QFrame::Sunken);
    }

    void setOrientation(Qt::Orientation orientation)
    {
        applyOrientation(orientation);
    }

    Qt::Orientation orientation() const
    {
        return m_orientation;
    }

private:
    void applyOrientation(Qt::Orientation orientation)
    {
        m_orientation = orientation;
        setFrameShape(m_orientation == Qt::Horizontal ? QFrame::HLine : QFrame::VLine);
    }

    Qt::Orientation m_orientation = Qt::Horizontal;
};
]=])

file(WRITE "${_external_include_dir}/kactionmenu.h" [=[
#pragma once

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QString>

class KActionMenu : public QAction
{
public:
    explicit KActionMenu(QObject *parent = nullptr)
        : QAction(parent)
    {
        ensureMenu(QString());
    }

    explicit KActionMenu(const QString &text, QObject *parent = nullptr)
        : QAction(text, parent)
    {
        ensureMenu(text);
    }

    KActionMenu(const QIcon &icon, const QString &text, QObject *parent = nullptr)
        : QAction(icon, text, parent)
    {
        ensureMenu(text);
    }

    void addAction(QAction *action)
    {
        menu()->addAction(action);
    }

    QAction *addAction(const QString &text)
    {
        return menu()->addAction(text);
    }

    QAction *addAction(const QIcon &icon, const QString &text)
    {
        return menu()->addAction(icon, text);
    }

    QAction *addSeparator()
    {
        return menu()->addSeparator();
    }

private:
    void ensureMenu(const QString &title)
    {
        if (!menu()) {
            setMenu(new QMenu(title));
        }
    }
};
]=])

file(WRITE "${_external_include_dir}/kstandardshortcut.h" [=[
#pragma once

#include <QKeySequence>
#include <QList>
#include <QString>

namespace KStandardShortcut {

enum StandardShortcut {
    AccelNone,
    New,
    Open,
    Save,
    SaveAs,
    Revert,
    Close,
    Print,
    PrintPreview,
    Mail,
    Quit,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Clear,
    SelectAll,
    Deselect,
    Find,
    FindNext,
    FindPrev,
    Replace,
    ActualSize,
    FitToPage,
    FitToWidth,
    FitToHeight,
    ZoomIn,
    ZoomOut,
    Zoom,
    Reload,
    Up,
    Back,
    Forward,
    Home,
    Prior,
    Next,
    Goto,
    GotoPage,
    GotoLine,
    Begin,
    End,
    DocumentBack,
    DocumentForward,
    AddBookmark,
    EditBookmarks,
    Spelling,
    ShowMenubar,
    ShowToolbar,
    ShowStatusbar,
    FullScreen,
    KeyBindings,
    Preferences,
    ConfigureToolbars,
    ConfigureNotifications,
    Help,
    WhatsThis,
    ReportBug,
    SwitchApplicationLanguage
};

inline QList<QKeySequence> shortcut(StandardShortcut)
{
    return {};
}

inline QList<QKeySequence> undo()
{
    return {};
}

inline QList<QKeySequence> redo()
{
    return {};
}

inline StandardShortcut find(const QKeySequence &)
{
    return AccelNone;
}

inline QString label(StandardShortcut shortcut)
{
    switch (shortcut) {
    case ShowToolbar:
        return QStringLiteral("Show Toolbar");
    case FullScreen:
        return QStringLiteral("Full Screen");
    default:
        return QString();
    }
}

} // namespace KStandardShortcut
]=])
file(WRITE "${_external_include_dir}/KStandardShortcut" [=[
#pragma once
#include "kstandardshortcut.h"
]=])

file(WRITE "${_external_include_dir}/kacceleratormanager.h" [=[
#pragma once

#include <QStringList>
#include <QWidget>

class KAcceleratorManager
{
public:
    static void addStandardActionNames(const QStringList &)
    {
    }

    static void manage(QWidget *)
    {
    }

    static void setNoAccel(QWidget *)
    {
    }

    static void last_manage(QWidget *, QWidget *, const QString &)
    {
    }

    static void last_manage(QString &, QString &, QString &)
    {
    }
};
]=])
file(WRITE "${_external_include_dir}/KAcceleratorManager" [=[
#pragma once
#include "kacceleratormanager.h"
]=])

file(WRITE "${_external_include_dir}/kselectaction.h" [=[
#pragma once

#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QMenu>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QWidgetAction>

class KSelectAction : public QWidgetAction
{
public:
    enum ToolBarMode {
        MenuMode
    };

    explicit KSelectAction(QObject *parent = nullptr)
        : QWidgetAction(parent)
        , m_selectableActionGroup(new QActionGroup(this))
    {
    }

    explicit KSelectAction(const QString &text, QObject *parent = nullptr)
        : QWidgetAction(parent)
        , m_selectableActionGroup(new QActionGroup(this))
    {
        setText(text);
    }

    KSelectAction(const QIcon &icon, const QString &text, QObject *parent = nullptr)
        : QWidgetAction(parent)
        , m_selectableActionGroup(new QActionGroup(this))
    {
        setIcon(icon);
        setText(text);
    }

    void setToolBarMode(ToolBarMode)
    {
    }

    void setEditable(bool editable)
    {
        m_editable = editable;
    }

    bool isEditable() const
    {
        return m_editable;
    }

    void setMaxComboViewCount(int count)
    {
        m_maxComboViewCount = count;
    }

    int maxComboViewCount() const
    {
        return m_maxComboViewCount;
    }

    void setItems(const QStringList &items)
    {
        clear();
        m_items = items;
        for (const QString &item : m_items) {
            QAction *action = new QAction(item, this);
            action->setCheckable(true);
            addAction(action);
        }
    }

    QStringList items() const
    {
        return m_items;
    }

    void setCurrentItem(int index)
    {
        m_currentItem = index;
        const auto actionList = m_selectableActionGroup->actions();
        if (index >= 0 && index < actionList.size()) {
            actionList.at(index)->setChecked(true);
        }
    }

    int currentItem() const
    {
        return m_currentItem;
    }

    QAction *currentAction() const
    {
        return m_selectableActionGroup->checkedAction();
    }

    void addAction(QAction *action)
    {
        ensureMenu();
        menu()->addAction(action);
        m_selectableActionGroup->addAction(action);
    }

    virtual QAction *removeAction(QAction *action)
    {
        if (menu()) {
            menu()->removeAction(action);
        }
        m_selectableActionGroup->removeAction(action);
        return action;
    }

    void clear()
    {
        if (menu()) {
            menu()->clear();
        }
        for (QAction *action : m_selectableActionGroup->actions()) {
            m_selectableActionGroup->removeAction(action);
        }
    }

    QActionGroup *selectableActionGroup() const
    {
        return m_selectableActionGroup;
    }

private:
    QWidget *createWidget(QWidget *) override
    {
        return nullptr;
    }

    void ensureMenu()
    {
        if (!menu()) {
            setMenu(new QMenu());
        }
    }

    QActionGroup *m_selectableActionGroup = nullptr;
    QStringList m_items;
    bool m_editable = false;
    int m_maxComboViewCount = 0;
    int m_currentItem = -1;
};
]=])
file(WRITE "${_external_include_dir}/KSelectAction" [=[
#pragma once
#include "kselectaction.h"
]=])

file(WRITE "${_external_include_dir}/kdesktopfile.h" [=[
#pragma once

#include <QString>

class KDesktopFile
{
public:
    explicit KDesktopFile(const QString &)
    {
    }

    QString readDocPath() const
    {
        return QString();
    }
};
]=])
file(WRITE "${_external_include_dir}/KDesktopFile" [=[
#pragma once
#include "kdesktopfile.h"
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
        Q_UNUSED(parent)
        Q_UNUSED(args)
        return nullptr;
    }
};
]=])
file(WRITE "${_external_include_dir}/KPluginFactory" [=[
#pragma once
#include "kpluginfactory.h"
]=])

file(WRITE "${_external_include_dir}/krandom.h" [=[
#pragma once

#include <QRandomGenerator>
#include <QString>

class KRandom
{
public:
    static int random()
    {
        return static_cast<int>(QRandomGenerator::global()->generate() & 0x7fffffffU);
    }

    static int random(int max)
    {
        return max > 0 ? QRandomGenerator::global()->bounded(max) : 0;
    }

    static QString randomString(int length)
    {
        static const char alphabet[] =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789";

        QString value;
        if (length > 0) {
            value.reserve(length);
        }

        for (int i = 0; i < length; ++i) {
            value.append(QLatin1Char(alphabet[QRandomGenerator::global()->bounded(int(sizeof(alphabet) - 1))]));
        }

        return value;
    }
};

class KRandomSequence
{
public:
    explicit KRandomSequence(long seed = 0)
        : m_generator(static_cast<quint32>(seed))
    {
    }

    quint32 getLong(quint32 max)
    {
        return max > 0 ? m_generator.bounded(max) : 0;
    }

    double getDouble()
    {
        return m_generator.generateDouble();
    }

private:
    QRandomGenerator m_generator;
};
]=])
file(WRITE "${_external_include_dir}/KRandom" [=[
#pragma once
#include "krandom.h"
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

file(WRITE "${_external_include_dir}/lager/state.hpp" [=[
#pragma once

#include <type_traits>
#include <utility>

namespace lager {

struct automatic_tag
{
};

template <typename T, typename Tag = automatic_tag>
class state
{
public:
    using value_type = T;

    state() = default;
    explicit state(const T &value)
        : m_value(value)
    {
    }

    T get() const
    {
        return m_value;
    }

    T operator()() const
    {
        return m_value;
    }

    T &operator*()
    {
        return m_value;
    }

    const T &operator*() const
    {
        return m_value;
    }

    T *operator->()
    {
        return &m_value;
    }

    const T *operator->() const
    {
        return &m_value;
    }

    void set(const T &value)
    {
        m_value = value;
    }

    template <typename Func>
    void bind(Func &&func) const
    {
        func(m_value);
    }

    template <typename Func>
    void watch(Func &&func) const
    {
        if constexpr (std::is_invocable_v<Func, const T &>) {
            func(m_value);
        } else if constexpr (std::is_invocable_v<Func>) {
            func();
        }
    }

    template <typename Member,
              typename Object = T,
              typename = std::enable_if_t<std::is_class<Object>::value || std::is_union<Object>::value>>
    auto operator[](Member Object::*member) const -> state<std::decay_t<Member>, Tag>
    {
        return state<std::decay_t<Member>, Tag>(m_value.*member);
    }

    template <typename Lens>
    auto zoom(const Lens &lens) const -> state<std::decay_t<decltype(lens.get(std::declval<const T &>()))>, Tag>
    {
        using Result = std::decay_t<decltype(lens.get(std::declval<const T &>()))>;
        return state<Result, Tag>(lens.get(m_value));
    }

    template <typename Func>
    auto map(Func &&func) const -> state<std::decay_t<decltype(func(std::declval<const T &>()))>, Tag>
    {
        using Result = std::decay_t<decltype(func(std::declval<const T &>()))>;
        return state<Result, Tag>(func(m_value));
    }

    template <typename Func>
    auto xform(Func &&func) const -> state<std::decay_t<decltype(func(std::declval<const T &>()))>, Tag>
    {
        return map(std::forward<Func>(func));
    }

    template <typename Func>
    void update(Func &&func)
    {
        m_value = func(m_value);
    }

private:
    T m_value {};
};

} // namespace lager
]=])

file(WRITE "${_external_include_dir}/lager/cursor.hpp" [=[
#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include "lenses.hpp"
#include "state.hpp"

namespace lager {

template <typename T>
class cursor
{
public:
    using value_type = T;

    cursor() = default;
    explicit cursor(const T &value)
        : m_value(value)
    {
    }

    template <typename Tag>
    cursor(const state<T, Tag> &state)
        : m_value(state.get())
    {
    }

    T get() const
    {
        return m_value;
    }

    T operator()() const
    {
        return m_value;
    }

    T &operator*()
    {
        return m_value;
    }

    const T &operator*() const
    {
        return m_value;
    }

    T *operator->()
    {
        return &m_value;
    }

    const T *operator->() const
    {
        return &m_value;
    }

    void set(const T &value)
    {
        m_value = value;
    }

    template <typename Func>
    void bind(Func &&func) const
    {
        func(m_value);
    }

    template <typename Func>
    void watch(Func &&func) const
    {
        if constexpr (std::is_invocable_v<Func, const T &>) {
            func(m_value);
        } else if constexpr (std::is_invocable_v<Func>) {
            func();
        }
    }

    template <typename Member,
              typename Object = T,
              typename = std::enable_if_t<std::is_class<Object>::value || std::is_union<Object>::value>>
    auto operator[](Member Object::*member) const -> cursor<std::decay_t<Member>>
    {
        return cursor<std::decay_t<Member>>(m_value.*member);
    }

    template <typename Lens>
    auto zoom(const Lens &lens) const -> cursor<std::decay_t<decltype(lens.get(std::declval<const T &>()))>>
    {
        using Result = std::decay_t<decltype(lens.get(std::declval<const T &>()))>;
        return cursor<Result>(lens.get(m_value));
    }

    template <typename Func>
    auto map(Func &&func) const -> cursor<std::decay_t<decltype(func(std::declval<const T &>()))>>
    {
        using Result = std::decay_t<decltype(func(std::declval<const T &>()))>;
        return cursor<Result>(func(m_value));
    }

    template <typename Func>
    auto xform(Func &&func) const -> cursor<std::decay_t<decltype(func(std::declval<const T &>()))>>
    {
        return map(std::forward<Func>(func));
    }

    template <typename Func>
    void update(Func &&func)
    {
        m_value = func(m_value);
    }

private:
    T m_value {};
};

template <typename T, typename Tag = automatic_tag>
cursor<T> make_state(const T &value, Tag = automatic_tag {})
{
    return cursor<T>(value);
}

template <typename Cursor, typename Func>
void watch(const Cursor &, Func &&)
{
}

template <typename T>
class qt_cursor
{
public:
    using value_type = T;

    qt_cursor() = default;

    template <typename State>
    explicit qt_cursor(const State &state)
        : m_value(state.get())
    {
    }

    template <typename Func>
    void bind(Func &&func)
    {
        func(m_value);
    }

    T operator()() const
    {
        return m_value;
    }

    T get() const
    {
        return m_value;
    }

    void set(const T &value)
    {
        m_value = value;
    }

    template <typename Func>
    auto map(Func &&func) const -> qt_cursor<std::decay_t<decltype(func(std::declval<const T &>()))>>
    {
        using Result = std::decay_t<decltype(func(std::declval<const T &>()))>;
        return qt_cursor<Result>(cursor<Result>(func(m_value)));
    }

    template <typename Func>
    auto xform(Func &&func) const -> qt_cursor<std::decay_t<decltype(func(std::declval<const T &>()))>>
    {
        return map(std::forward<Func>(func));
    }

private:
    T m_value {};
};

} // namespace lager
]=])

file(WRITE "${_external_include_dir}/lager/extra/qt.hpp" [=[
#pragma once

#include "../cursor.hpp"
#include "../reader.hpp"

#define LAGER_QT_CURSOR(type, name) \
    lager::qt_cursor<type> name; \
    void set##name(const type &_lagerValue) { this->name.set(_lagerValue); }
#define LAGER_QT_READER(type, name) \
    lager::reader<type> name;
#define LAGER_QT(name) name
]=])

file(WRITE "${_external_include_dir}/lager/constant.hpp" [=[
#pragma once

#include "cursor.hpp"

namespace lager {

template <typename T>
cursor<T> make_constant(const T &value)
{
    return cursor<T>(value);
}

} // namespace lager
]=])

file(WRITE "${_external_include_dir}/lager/reader.hpp" [=[
#pragma once

#include <functional>
#include <type_traits>
#include <tuple>
#include <utility>

#include "cursor.hpp"

namespace lager {

template <typename T>
class reader
{
public:
    using value_type = T;

    reader() = default;

    explicit reader(const T &value)
        : m_value(value)
    {
    }

    template <typename Source,
              typename = decltype(std::declval<const Source &>().get())>
    reader(const Source &source)
        : m_value(source.get())
    {
    }

    T get() const
    {
        return m_value;
    }

    T operator()() const
    {
        return m_value;
    }

    void set(const T &value)
    {
        m_value = value;
    }

    template <typename Source,
              typename = decltype(std::declval<const Source &>().get())>
    reader &operator=(const Source &source)
    {
        m_value = source.get();
        return *this;
    }

    template <typename Func>
    void bind(Func &&func) const
    {
        func(m_value);
    }

    template <typename Lens>
    auto zoom(const Lens &lens) const -> reader<std::decay_t<decltype(lens.get(std::declval<const T &>()))>>
    {
        using Result = std::decay_t<decltype(lens.get(std::declval<const T &>()))>;
        return reader<Result>(lens.get(m_value));
    }

    template <typename Func>
    auto map(Func &&func) const -> reader<std::decay_t<decltype(func(std::declval<const T &>()))>>
    {
        using Result = std::decay_t<decltype(func(std::declval<const T &>()))>;
        return reader<Result>(func(m_value));
    }

    template <typename Func>
    auto xform(Func &&func) const -> reader<std::decay_t<decltype(func(std::declval<const T &>()))>>
    {
        return map(std::forward<Func>(func));
    }

    template <typename Func>
    void watch(Func &&func) const
    {
        if constexpr (std::is_invocable_v<Func, const T &>) {
            func(m_value);
        } else if constexpr (std::is_invocable_v<Func>) {
            func();
        }
    }

private:
    T m_value {};
};

template <typename Source>
using source_value_t = typename std::decay_t<Source>::value_type;

template <typename... Args>
class with_result
{
public:
    using value_type = std::tuple<source_value_t<Args>...>;

    value_type get() const
    {
        return {};
    }

    template <typename Func>
    auto map(Func &&) const -> reader<std::decay_t<std::invoke_result_t<Func, source_value_t<Args>...>>>
    {
        return {};
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

#include <utility>

namespace lager {
namespace lenses {

template <typename Getter, typename Setter>
struct getset_lens
{
    Getter getter;
    Setter setter;

    template <typename Source>
    decltype(auto) get(Source &&source) const
    {
        return getter(std::forward<Source>(source));
    }

    template <typename Source, typename Value>
    decltype(auto) set(Source &&source, Value &&value) const
    {
        return setter(std::forward<Source>(source), std::forward<Value>(value));
    }
};

template <typename Getter, typename Setter>
getset_lens<Getter, Setter> getset(Getter getter, Setter setter)
{
    return {getter, setter};
}

template <typename Lhs, typename Rhs>
struct composed_lens
{
    Lhs lhs;
    Rhs rhs;

    template <typename Source>
    decltype(auto) get(Source &&source) const
    {
        return rhs.get(lhs.get(std::forward<Source>(source)));
    }

    template <typename Source, typename Value>
    decltype(auto) set(Source &&source, Value &&value) const
    {
        auto lhsValue = lhs.get(source);
        auto rhsValue = rhs.set(lhsValue, std::forward<Value>(value));
        return lhs.set(std::forward<Source>(source), rhsValue);
    }
};

template <typename Lhs, typename Rhs>
composed_lens<Lhs, Rhs> operator|(Lhs lhs, Rhs rhs)
{
    return {lhs, rhs};
}

} // namespace lenses
} // namespace lager
]=])

file(WRITE "${_external_include_dir}/lager/lenses/tuple.hpp" [=[
#pragma once

#include <cstddef>
#include <tuple>
#include <utility>

#include "../lenses.hpp"

namespace lager {
namespace lenses {

template <std::size_t Index>
struct tuple_lens
{
    template <typename Source>
    decltype(auto) get(Source &&source) const
    {
        return std::get<Index>(std::forward<Source>(source));
    }

    template <typename Source, typename Value>
    auto set(Source source, Value &&value) const
    {
        std::get<Index>(source) = std::forward<Value>(value);
        return source;
    }
};

inline constexpr tuple_lens<0> first {};
inline constexpr tuple_lens<1> second {};

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
constexpr decltype(auto) invoke(Func &&func, Args &&...args)
{
    return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
}

} // namespace compat
} // namespace zug
]=])

file(WRITE "${_external_include_dir}/zug/reducing/last.hpp" [=[
#pragma once

#include <utility>

namespace zug {

struct last_t
{
    template <typename Value>
    constexpr decltype(auto) operator()(Value &&value) const
    {
        return std::forward<Value>(value);
    }

    template <typename State, typename Value>
    constexpr decltype(auto) operator()(State &&, Value &&value) const
    {
        return std::forward<Value>(value);
    }
};

inline constexpr last_t last {};

} // namespace zug
]=])

file(WRITE "${_external_include_dir}/zug/transducer/map.hpp" [=[
#pragma once

#include <zug/compat/invoke.hpp>

#include <utility>

#ifndef ZUG_FWD
#define ZUG_FWD(x) std::forward<decltype(x)>(x)
#endif

namespace zug {

template <typename Func>
struct map_result
{
    Func func;

    template <typename Value>
    constexpr decltype(auto) operator()(Value &&value) const
    {
        return func(std::forward<Value>(value));
    }

    template <typename Reducer, typename Value>
    constexpr decltype(auto) operator()(Reducer &&reducer, Value &&value) const
    {
        return std::forward<Reducer>(reducer)(func(std::forward<Value>(value)));
    }
};

template <typename Func>
constexpr map_result<Func> map(Func func)
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

foreach(_kf_target IN ITEMS ItemViews ConfigGui GuiAddons WidgetsAddons Completion)
    add_library(KF5::${_kf_target} INTERFACE IMPORTED GLOBAL)
    set_target_properties(KF5::${_kf_target} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_external_include_dir}")
endforeach()

add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)
set_target_properties(ZLIB::ZLIB PROPERTIES
    INTERFACE_LINK_LIBRARIES z)

add_library(Boost::boost INTERFACE IMPORTED GLOBAL)
set_target_properties(Boost::boost PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include")

add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include/eigen3")

set(_seed_system_include_dirs "")
foreach(_seed_system_include_dir IN ITEMS
        "/usr/include"
        "/usr/include/freetype2"
        "/usr/include/harfbuzz"
        "/usr/include/fontconfig"
        "/usr/include/fribidi"
        "/usr/include/glib-2.0"
        "/usr/lib/x86_64-linux-gnu/glib-2.0/include")
    if(IS_DIRECTORY "${_seed_system_include_dir}")
        list(APPEND _seed_system_include_dirs "${_seed_system_include_dir}")
    endif()
endforeach()

set(_seed_unibreak_include_dir "")
foreach(_seed_unibreak_candidate IN ITEMS
        "/usr/include/libunibreak"
        "/usr/include/unibreak"
        "/usr/include")
    if(EXISTS "${_seed_unibreak_candidate}/graphemebreak.h")
        set(_seed_unibreak_include_dir "${_seed_unibreak_candidate}")
        break()
    endif()
endforeach()
if(NOT _seed_unibreak_include_dir)
    message(FATAL_ERROR "Missing libunibreak headers from libunibreak-dev")
endif()
list(APPEND _seed_system_include_dirs "${_seed_unibreak_include_dir}")

foreach(_seed_font_target IN ITEMS
        Freetype::Freetype
        HarfBuzz::HarfBuzz
        FriBidi::FriBidi
        Fontconfig::Fontconfig
        libunibreak::libunibreak)
    add_library(${_seed_font_target} INTERFACE IMPORTED GLOBAL)
    set_target_properties(${_seed_font_target} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_seed_system_include_dirs}")
endforeach()

set(_seed_text_dep_stubs_source "${CMAKE_CURRENT_BINARY_DIR}/seed_text_dep_stubs.c")
file(WRITE "${_seed_text_dep_stubs_source}" [=[
#include <stddef.h>
#include <stdint.h>

typedef void seed_void;
static unsigned int seed_count_zero = 0;

int FT_Bitmap_Embolden(void *library, void *bitmap, long xStrength, long yStrength) { (void)library; (void)bitmap; (void)xStrength; (void)yStrength; return 0; }
int FT_Done_Face(void *face) { (void)face; return 0; }
int FT_Done_FreeType(void *library) { (void)library; return 0; }
int FT_Done_MM_Var(void *library, void *amaster) { (void)library; (void)amaster; return 0; }
unsigned int FT_Get_Char_Index(void *face, unsigned long charcode) { (void)face; (void)charcode; return 0; }
int FT_Get_Color_Glyph_Layer(void *face, unsigned int baseGlyph, unsigned int *aglyphIndex, unsigned int *acolorIndex, void *iterator) { (void)face; (void)baseGlyph; (void)iterator; if (aglyphIndex) *aglyphIndex = 0; if (acolorIndex) *acolorIndex = 0; return 0; }
unsigned long FT_Get_First_Char(void *face, unsigned int *agindex) { (void)face; if (agindex) *agindex = 0; return 0; }
int FT_Get_MM_Var(void *face, void **amaster) { (void)face; if (amaster) *amaster = NULL; return 1; }
unsigned long FT_Get_Next_Char(void *face, unsigned long charcode, unsigned int *agindex) { (void)face; (void)charcode; if (agindex) *agindex = 0; return 0; }
const char *FT_Get_Postscript_Name(void *face) { (void)face; return ""; }
void *FT_Get_Sfnt_Table(void *face, unsigned int tag) { (void)face; (void)tag; return NULL; }
void FT_Get_Transform(void *face, void *matrix, void *delta) { (void)face; (void)matrix; (void)delta; }
int FT_GlyphSlot_Own_Bitmap(void *slot) { (void)slot; return 0; }
int FT_Init_FreeType(void **alibrary) { if (alibrary) *alibrary = NULL; return 0; }
int FT_Load_Glyph(void *face, unsigned int glyphIndex, int loadFlags) { (void)face; (void)glyphIndex; (void)loadFlags; return 1; }
long FT_MulFix(long a, long b) { return (a * b) / 65536; }
int FT_New_Face(void *library, const char *filepathname, long faceIndex, void **aface) { (void)library; (void)filepathname; (void)faceIndex; if (aface) *aface = NULL; return 1; }
int FT_Outline_Embolden(void *outline, long strength) { (void)outline; (void)strength; return 0; }
int FT_Palette_Select(void *face, unsigned short paletteIndex, void *apalette) { (void)face; (void)paletteIndex; (void)apalette; return 1; }
int FT_Reference_Face(void *face) { (void)face; return 0; }
int FT_Render_Glyph(void *slot, int renderMode) { (void)slot; (void)renderMode; return 1; }
int FT_Select_Size(void *face, int strikeIndex) { (void)face; (void)strikeIndex; return 1; }
int FT_Set_Charmap(void *face, void *charmap) { (void)face; (void)charmap; return 1; }
int FT_Set_Char_Size(void *face, long charWidth, long charHeight, unsigned int horzResolution, unsigned int vertResolution) { (void)face; (void)charWidth; (void)charHeight; (void)horzResolution; (void)vertResolution; return 1; }
void FT_Set_Transform(void *face, void *matrix, void *delta) { (void)face; (void)matrix; (void)delta; }
int FT_Set_Var_Design_Coordinates(void *face, unsigned int numCoords, long *coords) { (void)face; (void)numCoords; (void)coords; return 1; }
void FT_Vector_Transform(void *vector, const void *matrix) { (void)vector; (void)matrix; }

void hb_blob_destroy(void *blob) { (void)blob; }
const char *hb_blob_get_data(void *blob, unsigned int *length) { (void)blob; if (length) *length = 0; return ""; }
unsigned int hb_blob_get_length(void *blob) { (void)blob; return 0; }
void hb_buffer_add_utf32(void *buffer, const uint32_t *text, int textLength, unsigned int itemOffset, int itemLength) { (void)buffer; (void)text; (void)textLength; (void)itemOffset; (void)itemLength; }
void *hb_buffer_create(void) { return NULL; }
void hb_buffer_destroy(void *buffer) { (void)buffer; }
void *hb_buffer_get_glyph_infos(void *buffer, unsigned int *length) { (void)buffer; if (length) *length = 0; return NULL; }
void *hb_buffer_get_glyph_positions(void *buffer, unsigned int *length) { (void)buffer; if (length) *length = 0; return NULL; }
unsigned int hb_buffer_get_length(void *buffer) { (void)buffer; return 0; }
void hb_buffer_reset(void *buffer) { (void)buffer; }
void hb_buffer_set_direction(void *buffer, int direction) { (void)buffer; (void)direction; }
void hb_buffer_set_flags(void *buffer, int flags) { (void)buffer; (void)flags; }
void hb_buffer_set_invisible_glyph(void *buffer, unsigned int glyph) { (void)buffer; (void)glyph; }
void hb_buffer_set_language(void *buffer, void *language) { (void)buffer; (void)language; }
void hb_buffer_set_script(void *buffer, int script) { (void)buffer; (void)script; }
const char *hb_direction_to_string(int direction) { (void)direction; return "ltr"; }
void hb_face_collect_variation_selectors(void *face, void *out) { (void)face; (void)out; }
void hb_face_collect_variation_unicodes(void *face, unsigned int variationSelector, void *out) { (void)face; (void)variationSelector; (void)out; }
void hb_face_destroy(void *face) { (void)face; }
int hb_feature_from_string(const char *string, int length, void *feature) { (void)string; (void)length; (void)feature; return 0; }
void hb_font_destroy(void *font) { (void)font; }
void *hb_ft_face_create_referenced(void *ftFace) { (void)ftFace; return NULL; }
void *hb_ft_font_create_referenced(void *ftFace) { (void)ftFace; return NULL; }
void *hb_ft_font_get_face(void *font) { (void)font; return NULL; }
void hb_ft_font_set_load_flags(void *font, int loadFlags) { (void)font; (void)loadFlags; }
void *hb_language_from_string(const char *string, int length) { (void)string; (void)length; return NULL; }
void *hb_language_get_default(void) { return NULL; }
const char *hb_language_to_string(void *language) { (void)language; return ""; }
int hb_ot_color_has_layers(void *face) { (void)face; return 0; }
int hb_ot_color_has_paint(void *face) { (void)face; return 0; }
int hb_ot_color_has_png(void *face) { (void)face; return 0; }
int hb_ot_color_has_svg(void *face) { (void)face; return 0; }
unsigned int hb_ot_layout_feature_get_lookups(void *face, unsigned int tableTag, unsigned int featureIndex, unsigned int startOffset, unsigned int *lookupCount, unsigned int *lookupIndexes) { (void)face; (void)tableTag; (void)featureIndex; (void)startOffset; (void)lookupIndexes; if (lookupCount) *lookupCount = 0; return 0; }
int hb_ot_layout_feature_get_name_ids(void *face, unsigned int tableTag, unsigned int featureIndex, unsigned int *labelId, unsigned int *tooltipId, unsigned int *sampleId, unsigned int *numNamedParametersId, unsigned int *firstParamId) { (void)face; (void)tableTag; (void)featureIndex; if (labelId) *labelId = 0; if (tooltipId) *tooltipId = 0; if (sampleId) *sampleId = 0; if (numNamedParametersId) *numNamedParametersId = 0; if (firstParamId) *firstParamId = 0; return 0; }
int hb_ot_layout_get_baseline(void *font, unsigned int baselineTag, int direction, unsigned int scriptTag, unsigned int languageTag, int *coord) { (void)font; (void)baselineTag; (void)direction; (void)scriptTag; (void)languageTag; if (coord) *coord = 0; return 0; }
void hb_ot_layout_get_baseline_with_fallback(void *font, unsigned int baselineTag, int direction, unsigned int scriptTag, unsigned int languageTag, int *coord) { (void)font; (void)baselineTag; (void)direction; (void)scriptTag; (void)languageTag; if (coord) *coord = 0; }
unsigned int hb_ot_layout_get_ligature_carets(void *font, int direction, unsigned int glyph, unsigned int startOffset, unsigned int *caretCount, int *caretArray) { (void)font; (void)direction; (void)glyph; (void)startOffset; (void)caretArray; if (caretCount) *caretCount = 0; return 0; }
int hb_ot_layout_get_size_params(void *face, unsigned int *designSize, unsigned int *subfamilyId, unsigned int *subfamilyNameId, unsigned int *rangeStart, unsigned int *rangeEnd) { (void)face; if (designSize) *designSize = 0; if (subfamilyId) *subfamilyId = 0; if (subfamilyNameId) *subfamilyNameId = 0; if (rangeStart) *rangeStart = 0; if (rangeEnd) *rangeEnd = 0; return 0; }
int hb_ot_layout_language_find_feature(void *face, unsigned int tableTag, unsigned int scriptIndex, unsigned int languageIndex, unsigned int featureTag, unsigned int *featureIndex) { (void)face; (void)tableTag; (void)scriptIndex; (void)languageIndex; (void)featureTag; if (featureIndex) *featureIndex = 0; return 0; }
unsigned int hb_ot_layout_language_get_feature_tags(void *face, unsigned int tableTag, unsigned int scriptIndex, unsigned int languageIndex, unsigned int startOffset, unsigned int *featureCount, unsigned int *featureTags) { (void)face; (void)tableTag; (void)scriptIndex; (void)languageIndex; (void)startOffset; (void)featureTags; if (featureCount) *featureCount = 0; return 0; }
void hb_ot_layout_lookup_collect_glyphs(void *face, unsigned int tableTag, unsigned int lookupIndex, void *glyphsBefore, void *glyphsInput, void *glyphsAfter, void *glyphsOutput) { (void)face; (void)tableTag; (void)lookupIndex; (void)glyphsBefore; (void)glyphsInput; (void)glyphsAfter; (void)glyphsOutput; }
unsigned int hb_ot_layout_lookup_get_glyph_alternates(void *face, unsigned int lookupIndex, unsigned int glyph, unsigned int startOffset, unsigned int *alternateCount, unsigned int *alternateGlyphs) { (void)face; (void)lookupIndex; (void)glyph; (void)startOffset; (void)alternateGlyphs; if (alternateCount) *alternateCount = 0; return 0; }
unsigned int hb_ot_layout_script_get_language_tags(void *face, unsigned int tableTag, unsigned int scriptIndex, unsigned int startOffset, unsigned int *languageCount, unsigned int *languageTags) { (void)face; (void)tableTag; (void)scriptIndex; (void)startOffset; (void)languageTags; if (languageCount) *languageCount = 0; return 0; }
int hb_ot_layout_script_select_language(void *face, unsigned int tableTag, unsigned int scriptIndex, unsigned int languageCount, const unsigned int *languageTags, unsigned int *languageIndex) { (void)face; (void)tableTag; (void)scriptIndex; (void)languageCount; (void)languageTags; if (languageIndex) *languageIndex = 0; return 0; }
unsigned int hb_ot_layout_table_get_script_tags(void *face, unsigned int tableTag, unsigned int startOffset, unsigned int *scriptCount, unsigned int *scriptTags) { (void)face; (void)tableTag; (void)startOffset; (void)scriptTags; if (scriptCount) *scriptCount = 0; return 0; }
int hb_ot_layout_table_select_script(void *face, unsigned int tableTag, unsigned int scriptCount, const unsigned int *scriptTags, unsigned int *scriptIndex, unsigned int *chosenScript) { (void)face; (void)tableTag; (void)scriptCount; (void)scriptTags; if (scriptIndex) *scriptIndex = 0; if (chosenScript) *chosenScript = 0; return 0; }
void *hb_ot_meta_reference_entry(void *face, unsigned int metaTag) { (void)face; (void)metaTag; return NULL; }
int hb_ot_metrics_get_position(void *font, unsigned int metricsTag, int *position) { (void)font; (void)metricsTag; if (position) *position = 0; return 0; }
void hb_ot_metrics_get_position_with_fallback(void *font, unsigned int metricsTag, int *position) { (void)font; (void)metricsTag; if (position) *position = 0; }
unsigned int hb_ot_name_get_utf8(void *face, unsigned int nameId, void *language, unsigned int *textSize, char *text) { (void)face; (void)nameId; (void)language; if (textSize) *textSize = 0; if (text) *text = '\0'; return 0; }
const void *hb_ot_name_list_names(void *face, unsigned int *count) { (void)face; if (count) *count = 0; return NULL; }
void *hb_ot_tag_to_language(unsigned int tag) { (void)tag; return NULL; }
void hb_ot_tags_from_script_and_language(int script, void *language, unsigned int *scriptCount, unsigned int *scriptTags, unsigned int *languageCount, unsigned int *languageTags) { (void)script; (void)language; if (scriptCount) *scriptCount = 0; if (scriptTags) *scriptTags = 0; if (languageCount) *languageCount = 0; if (languageTags) *languageTags = 0; }
unsigned int hb_ot_var_get_axis_count(void *face) { (void)face; return 0; }
unsigned int hb_ot_var_get_axis_infos(void *face, unsigned int startOffset, unsigned int *axisCount, void *axisArray) { (void)face; (void)startOffset; (void)axisArray; if (axisCount) *axisCount = 0; return 0; }
unsigned int hb_ot_var_get_named_instance_count(void *face) { (void)face; return 0; }
int hb_ot_var_has_data(void *face) { (void)face; return 0; }
unsigned int hb_ot_var_named_instance_get_design_coords(void *face, unsigned int instanceIndex, unsigned int *coordCount, float *coords) { (void)face; (void)instanceIndex; (void)coords; if (coordCount) *coordCount = 0; return 0; }
unsigned int hb_ot_var_named_instance_get_subfamily_name_id(void *face, unsigned int instanceIndex) { (void)face; (void)instanceIndex; return 0; }
int hb_script_from_string(const char *string, int length) { (void)string; (void)length; return 0; }
unsigned int hb_script_to_iso15924_tag(int script) { (void)script; return 0; }
void *hb_set_create(void) { return NULL; }
void hb_set_destroy(void *set) { (void)set; }
int hb_set_next(void *set, unsigned int *codepoint) { (void)set; if (codepoint) *codepoint = 0; return 0; }
int hb_shape_full(void *font, void *buffer, const void *features, unsigned int numFeatures, const char * const *shaperList) { (void)font; (void)buffer; (void)features; (void)numFeatures; (void)shaperList; return 0; }
float hb_style_get_value(void *font, unsigned int styleTag) { (void)font; (void)styleTag; return 0.0f; }
unsigned int hb_tag_from_string(const char *string, int length) { (void)string; (void)length; return 0; }
void hb_tag_to_string(unsigned int tag, char *buf) { (void)tag; if (buf) { buf[0] = '\0'; } }
void *hb_unicode_funcs_get_default(void) { return NULL; }
int hb_unicode_general_category(void *funcs, unsigned int unicode) { (void)funcs; (void)unicode; return 0; }
int hb_unicode_script(void *funcs, unsigned int unicode) { (void)funcs; (void)unicode; return 0; }
int hb_version_atleast(unsigned int major, unsigned int minor, unsigned int micro) { (void)major; (void)minor; (void)micro; return 0; }

void set_graphemebreaks_utf16(const void *text, size_t length, const char *language, char *breaks) { (void)text; (void)language; if (breaks) { for (size_t i = 0; i < length; ++i) breaks[i] = 0; } }
void set_linebreaks_utf16(const void *text, size_t length, const char *language, char *breaks) { (void)text; (void)language; if (breaks) { for (size_t i = 0; i < length; ++i) breaks[i] = 0; } }
void set_wordbreaks_utf16(const void *text, size_t length, const char *language, char *breaks) { (void)text; (void)language; if (breaks) { for (size_t i = 0; i < length; ++i) breaks[i] = 0; } }

int fribidi_get_par_embedding_levels_ex(const void *bidiTypes, const void *bracketTypes, unsigned int len, void *baseDir, void *embeddingLevels) { (void)bidiTypes; (void)bracketTypes; (void)len; (void)baseDir; (void)embeddingLevels; return 0; }
void fribidi_get_bidi_types(const uint32_t *str, unsigned int len, void *types) { (void)str; (void)len; (void)types; }
void fribidi_get_bracket_types(const uint32_t *str, unsigned int len, const void *types, void *bracketTypes) { (void)str; (void)len; (void)types; (void)bracketTypes; }

int FcCharSetHasChar(void *charset, unsigned int ucs4) { (void)charset; (void)ucs4; return 0; }
void FcCharSetDestroy(void *charset) { (void)charset; }
int FcConfigAppFontAddDir(void *config, const unsigned char *dir) { (void)config; (void)dir; return 0; }
int FcConfigAppFontAddFile(void *config, const unsigned char *file) { (void)config; (void)file; return 0; }
int FcConfigBuildFonts(void *config) { (void)config; return 1; }
void *FcConfigCreate(void) { return NULL; }
void FcConfigDestroy(void *config) { (void)config; }
void *FcConfigGetCurrent(void) { return NULL; }
void *FcConfigGetFontDirs(void *config) { (void)config; return NULL; }
int FcConfigParseAndLoad(void *config, const unsigned char *file, int complain) { (void)config; (void)file; (void)complain; return 0; }
int FcConfigSetCurrent(void *config) { (void)config; return 1; }
int FcConfigSubstitute(void *config, void *pattern, int kind) { (void)config; (void)pattern; (void)kind; return 1; }
void FcDefaultSubstitute(void *pattern) { (void)pattern; }
void *FcFontList(void *config, void *pattern, void *objectSet) { (void)config; (void)pattern; (void)objectSet; return NULL; }
void *FcFontMatch(void *config, void *pattern, int *result) { (void)config; (void)pattern; if (result) *result = 1; return NULL; }
void *FcFontSort(void *config, void *pattern, int trim, void **charset, int *result) { (void)config; (void)pattern; (void)trim; if (charset) *charset = NULL; if (result) *result = 1; return NULL; }
void FcFontSetDestroy(void *fontSet) { (void)fontSet; }
void *FcLangSetGetLangs(void *langSet) { (void)langSet; return NULL; }
void *FcObjectSetBuild(const char *first, ...) { (void)first; return NULL; }
int FcPatternAddDouble(void *pattern, const char *object, double value) { (void)pattern; (void)object; (void)value; return 1; }
int FcPatternAddInteger(void *pattern, const char *object, int value) { (void)pattern; (void)object; (void)value; return 1; }
int FcPatternAddString(void *pattern, const char *object, const unsigned char *value) { (void)pattern; (void)object; (void)value; return 1; }
int FcPatternAddWeak(void *pattern, const char *object, int weak) { (void)pattern; (void)object; (void)weak; return 1; }
void *FcPatternCreate(void) { return NULL; }
void FcPatternDestroy(void *pattern) { (void)pattern; }
int FcPatternGetBool(void *pattern, const char *object, int n, int *value) { (void)pattern; (void)object; (void)n; if (value) *value = 0; return 1; }
int FcPatternGetCharSet(void *pattern, const char *object, int n, void **value) { (void)pattern; (void)object; (void)n; if (value) *value = NULL; return 1; }
int FcPatternGetDouble(void *pattern, const char *object, int n, double *value) { (void)pattern; (void)object; (void)n; if (value) *value = 0.0; return 1; }
int FcPatternGetInteger(void *pattern, const char *object, int n, int *value) { (void)pattern; (void)object; (void)n; if (value) *value = 0; return 1; }
int FcPatternGetLangSet(void *pattern, const char *object, int n, void **value) { (void)pattern; (void)object; (void)n; if (value) *value = NULL; return 1; }
int FcPatternGetString(void *pattern, const char *object, int n, unsigned char **value) { (void)pattern; (void)object; (void)n; if (value) *value = NULL; return 1; }
unsigned int FcPatternHash(const void *pattern) { (void)pattern; return seed_count_zero; }
void *FcStrListCreate(const unsigned char **strings) { (void)strings; return NULL; }
void FcStrListDone(void *list) { (void)list; }
unsigned char *FcStrListFirst(void *list) { (void)list; return NULL; }
unsigned char *FcStrListNext(void *list) { (void)list; return NULL; }
int FcWeightFromOpenType(int otWeight) { return otWeight; }
int FcWeightToOpenType(int fcWeight) { return fcWeight; }

seed_void seed_text_dep_stubs_anchor(void) {}
]=])
add_library(seed_text_dep_stubs STATIC "${_seed_text_dep_stubs_source}")
set_property(TARGET seed_text_dep_stubs PROPERTY C_STANDARD 99)

set(_seed_png_dep_stubs_source "${CMAKE_CURRENT_BINARY_DIR}/seed_png_dep_stubs.c")
file(WRITE "${_seed_png_dep_stubs_source}" [=[
#include "png.h"

#include <stdlib.h>
#include <string.h>

static png_color seed_png_palette[1] = {{0, 0, 0}};

int png_check_sig(png_bytep sig, int num) { (void)sig; (void)num; return 0; }
int png_sig_cmp(png_bytep sig, png_size_t start, png_size_t num_to_check) { (void)sig; (void)start; (void)num_to_check; return 1; }

png_structp png_create_read_struct(png_const_charp user_png_ver, void *error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)
{
    (void)user_png_ver; (void)error_ptr; (void)error_fn; (void)warn_fn;
    return (png_structp)calloc(1, sizeof(png_struct));
}

png_structp png_create_write_struct(png_const_charp user_png_ver, void *error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)
{
    (void)user_png_ver; (void)error_ptr; (void)error_fn; (void)warn_fn;
    return (png_structp)calloc(1, sizeof(png_struct));
}

png_infop png_create_info_struct(png_structp png_ptr)
{
    (void)png_ptr;
    return (png_infop)calloc(1, sizeof(png_info));
}

void png_destroy_read_struct(png_structp *png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr)
{
    if (png_ptr_ptr && *png_ptr_ptr) { free(*png_ptr_ptr); *png_ptr_ptr = NULL; }
    if (info_ptr_ptr && *info_ptr_ptr) { free(*info_ptr_ptr); *info_ptr_ptr = NULL; }
    if (end_info_ptr_ptr && *end_info_ptr_ptr) { free(*end_info_ptr_ptr); *end_info_ptr_ptr = NULL; }
}

void png_destroy_write_struct(png_structp *png_ptr_ptr, png_infopp info_ptr_ptr)
{
    if (png_ptr_ptr && *png_ptr_ptr) { free(*png_ptr_ptr); *png_ptr_ptr = NULL; }
    if (info_ptr_ptr && *info_ptr_ptr) { free(*info_ptr_ptr); *info_ptr_ptr = NULL; }
}

void png_set_error_fn(png_structp png_ptr, void *error_ptr, png_error_ptr error_fn, png_error_ptr warning_fn)
{
    (void)png_ptr; (void)error_ptr; (void)error_fn; (void)warning_fn;
}

void png_set_read_fn(png_structp png_ptr, void *io_ptr, png_rw_ptr read_data_fn)
{
    (void)read_data_fn;
    if (png_ptr) png_ptr->io_ptr = io_ptr;
}

void png_set_write_fn(png_structp png_ptr, void *io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
    (void)write_data_fn; (void)output_flush_fn;
    if (png_ptr) png_ptr->io_ptr = io_ptr;
}

void *png_get_io_ptr(png_structp png_ptr) { return png_ptr ? png_ptr->io_ptr : NULL; }
void png_error(png_structp png_ptr, png_const_charp message) { (void)png_ptr; (void)message; }
void *png_malloc(png_structp png_ptr, png_uint_32 size) { (void)png_ptr; return calloc(1, size ? size : 1); }
void png_free(png_structp png_ptr, void *ptr) { (void)png_ptr; free(ptr); }
void png_read_info(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; }
void png_read_update_info(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; }
void png_read_row(png_structp png_ptr, png_bytep row, png_bytep display_row) { (void)png_ptr; if (row) memset(row, 0, 1); if (display_row) memset(display_row, 0, 1); }
void png_read_image(png_structp png_ptr, png_bytepp image) { (void)png_ptr; (void)image; }
void png_read_end(png_structp png_ptr, png_infop end_info) { (void)png_ptr; (void)end_info; }
void png_write_info(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; }
void png_write_flush(png_structp png_ptr) { (void)png_ptr; }
void png_write_image(png_structp png_ptr, png_bytepp image) { (void)png_ptr; (void)image; }
void png_write_end(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; }
void png_set_expand(png_structp png_ptr) { (void)png_ptr; }
void png_set_packing(png_structp png_ptr) { (void)png_ptr; }
void png_set_swap(png_structp png_ptr) { (void)png_ptr; }
void png_set_compression_level(png_structp png_ptr, int level) { (void)png_ptr; (void)level; }
void png_set_compression_mem_level(png_structp png_ptr, int mem_level) { (void)png_ptr; (void)mem_level; }
void png_set_compression_strategy(png_structp png_ptr, int strategy) { (void)png_ptr; (void)strategy; }
void png_set_compression_window_bits(png_structp png_ptr, int window_bits) { (void)png_ptr; (void)window_bits; }
void png_set_compression_method(png_structp png_ptr, int method) { (void)png_ptr; (void)method; }
void png_set_compression_buffer_size(png_structp png_ptr, png_size_t size) { (void)png_ptr; (void)size; }
png_size_t png_get_rowbytes(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; return 1; }
int png_get_color_type(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; return PNG_COLOR_TYPE_RGB_ALPHA; }
int png_get_bit_depth(png_structp png_ptr, png_infop info_ptr) { (void)png_ptr; (void)info_ptr; return 8; }
png_uint_32 png_get_valid(png_structp png_ptr, png_infop info_ptr, png_uint_32 flag) { (void)png_ptr; (void)info_ptr; (void)flag; return 0; }

png_uint_32 png_get_IHDR(png_structp png_ptr, png_infop info_ptr, png_uint_32 *width, png_uint_32 *height, int *bit_depth, int *color_type, int *interlace_type, int *compression_type, int *filter_method)
{
    (void)png_ptr; (void)info_ptr;
    if (width) *width = 0;
    if (height) *height = 0;
    if (bit_depth) *bit_depth = 8;
    if (color_type) *color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    if (interlace_type) *interlace_type = PNG_INTERLACE_NONE;
    if (compression_type) *compression_type = PNG_COMPRESSION_TYPE_DEFAULT;
    if (filter_method) *filter_method = PNG_FILTER_TYPE_DEFAULT;
    return 1;
}

int png_get_iCCP(png_structp png_ptr, png_infop info_ptr, png_charpp name, int *compression_type, png_bytepp profile, png_uint_32 *proflen)
{
    (void)png_ptr; (void)info_ptr;
    if (name) *name = NULL;
    if (compression_type) *compression_type = 0;
    if (profile) *profile = NULL;
    if (proflen) *proflen = 0;
    return 0;
}

int png_get_text(png_structp png_ptr, png_infop info_ptr, png_textp *text_ptr, int *num_text)
{
    (void)png_ptr; (void)info_ptr;
    if (text_ptr) *text_ptr = NULL;
    if (num_text) *num_text = 0;
    return 0;
}

int png_get_pHYs(png_structp png_ptr, png_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type)
{
    (void)png_ptr; (void)info_ptr;
    if (res_x) *res_x = 0;
    if (res_y) *res_y = 0;
    if (unit_type) *unit_type = PNG_RESOLUTION_UNKNOWN;
    return 0;
}

int png_get_PLTE(png_structp png_ptr, png_infop info_ptr, png_colorp *palette, int *num_palette)
{
    (void)png_ptr; (void)info_ptr;
    if (palette) *palette = seed_png_palette;
    if (num_palette) *num_palette = 0;
    return 0;
}

int png_get_tRNS(png_structp png_ptr, png_infop info_ptr, png_bytep *trans_alpha, int *num_trans, void *trans_color)
{
    (void)png_ptr; (void)info_ptr; (void)trans_color;
    if (trans_alpha) *trans_alpha = NULL;
    if (num_trans) *num_trans = 0;
    return 0;
}

void png_set_IHDR(png_structp png_ptr, png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type, int compression_type, int filter_method)
{
    (void)png_ptr; (void)info_ptr; (void)width; (void)height; (void)bit_depth; (void)color_type; (void)interlace_type; (void)compression_type; (void)filter_method;
}
void png_set_PLTE(png_structp png_ptr, png_infop info_ptr, png_colorp palette, int num_palette) { (void)png_ptr; (void)info_ptr; (void)palette; (void)num_palette; }
void png_set_text(png_structp png_ptr, png_infop info_ptr, png_textp text_ptr, int num_text) { (void)png_ptr; (void)info_ptr; (void)text_ptr; (void)num_text; }
void png_set_iCCP(png_structp png_ptr, png_infop info_ptr, png_const_charp name, int compression_type, png_const_bytep profile, png_uint_32 proflen) { (void)png_ptr; (void)info_ptr; (void)name; (void)compression_type; (void)profile; (void)proflen; }
void png_set_pHYs(png_structp png_ptr, png_infop info_ptr, png_uint_32 res_x, png_uint_32 res_y, int unit_type) { (void)png_ptr; (void)info_ptr; (void)res_x; (void)res_y; (void)unit_type; }
void png_set_sRGB_gAMA_and_cHRM(png_structp png_ptr, png_infop info_ptr, int intent) { (void)png_ptr; (void)info_ptr; (void)intent; }
]=])
add_library(seed_png_dep_stubs STATIC "${_seed_png_dep_stubs_source}")
set_property(TARGET seed_png_dep_stubs PROPERTY C_STANDARD 99)
target_include_directories(seed_png_dep_stubs PRIVATE "${_external_include_dir}")
set(PNG_LIBRARIES seed_png_dep_stubs)
set(PNG_INCLUDE_DIR "${_external_include_dir}")
set(PNG_FOUND TRUE)

add_library(xsimd INTERFACE)
target_include_directories(xsimd INTERFACE "${_external_include_dir}")

function(seed_xsimd_compile_one out_var source implementation)
    get_filename_component(_source_name "${source}" NAME_WE)
    get_filename_component(_source_ext "${source}" EXT)
    string(REPLACE "+" "_plus_" _safe_implementation "${implementation}")
    set(_generated_source "${CMAKE_CURRENT_BINARY_DIR}/${_source_name}_${_safe_implementation}${_source_ext}")

    add_custom_command(
        OUTPUT "${_generated_source}"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${source}" "${_generated_source}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
        VERBATIM)

    set(_compile_flags "")
    if(implementation STREQUAL "SSE2")
        set(_compile_flags "-msse2")
    elseif(implementation STREQUAL "SSSE3")
        set(_compile_flags "-mssse3")
    elseif(implementation STREQUAL "SSE4_1")
        set(_compile_flags "-msse4.1")
    elseif(implementation STREQUAL "AVX")
        set(_compile_flags "-mavx")
    elseif(implementation STREQUAL "AVX2+FMA")
        set(_compile_flags "-mavx2 -mfma")
    endif()

    set_source_files_properties("${_generated_source}" PROPERTIES
        COMPILE_DEFINITIONS "XSIMD_IMPL=${implementation}"
        COMPILE_FLAGS "${_compile_flags}"
        SKIP_PRECOMPILE_HEADERS TRUE)

    set(${out_var} ${${out_var}} "${_generated_source}" PARENT_SCOPE)
endfunction()

function(seed_xsimd_compile_for_all_implementations out_var source)
    set(_implementations ${ARGN})
    foreach(_implementation IN LISTS _implementations)
        seed_xsimd_compile_one(_generated_sources "${source}" "${_implementation}")
    endforeach()

    set(${out_var} ${${out_var}} ${_generated_sources} PARENT_SCOPE)
endfunction()

macro(ko_compile_for_all_implementations_no_scalar _objs _src)
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        seed_xsimd_compile_for_all_implementations(${_objs} ${_src} NEON64)
    elseif(ANDROID_ABI STREQUAL "x86_64")
        seed_xsimd_compile_for_all_implementations(${_objs} ${_src} SSE2 SSSE3 SSE4_1 AVX AVX2+FMA)
    endif()
endmacro()

macro(ko_compile_for_all_implementations _objs _src)
    seed_xsimd_compile_for_all_implementations(${_objs} ${_src} Scalar)
    ko_compile_for_all_implementations_no_scalar(${_objs} ${_src})
endmacro()

add_library(lager INTERFACE)
target_include_directories(lager INTERFACE "${_external_include_dir}")

add_library(zug INTERFACE)
target_include_directories(zug INTERFACE "${_external_include_dir}")

set(HIDE_SAFE_ASSERTS ON)
set(CRASH_ON_SAFE_ASSERTS OFF)
set(HAVE_DBUS OFF)
set(GSL_FOUND OFF)
set(HAVE_GSL 0)
set(ENABLE_UPDATERS OFF)
set(USE_LOCK_FREE_HASH_TABLE OFF)
set(ANDROID_ENABLE_STDIO_FORWARDING OFF)
set(QT_QTCORE_LIBRARY Qt5::Core)
set(QUAZIP_INCLUDE_DIRS "${_external_include_dir}")
set(QUAZIP_LIBRARIES "")
configure_file("${KRITA_SOURCE_DIR}/KoConfig.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/KoConfig.h")
configure_file("${KRITA_SOURCE_DIR}/config-hdr.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-hdr.h")
configure_file("${KRITA_SOURCE_DIR}/config-ocio.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-ocio.h")
configure_file("${KRITA_SOURCE_DIR}/config_convolution.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config_convolution.h")
configure_file("${KRITA_SOURCE_DIR}/config-hash-table-implementation.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-hash-table-implementation.h")
configure_file("${KRITA_SOURCE_DIR}/config-qt-patches-present.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-qt-patches-present.h")
configure_file("${KRITA_SOURCE_DIR}/config-updaters.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-updaters.h")
configure_file("${KRITA_SOURCE_DIR}/config-use-surface-color-management-api.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config-use-surface-color-management-api.h")
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

disable_seed_tests(libs/pigment)
disable_seed_subdir(libs/pigment/benchmarks)

set(_kritapigment_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritapigment")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/pigment" "${_kritapigment_build_dir}")

target_include_directories(kritapigment
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/pigment>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/pigment/resources>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/pigment/compositeops>"
        "$<BUILD_INTERFACE:${_kritapigment_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritapigment PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritapigment PRIVATE
    -fno-operator-names)

disable_seed_tests(libs/metadata)

set(_kritametadata_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritametadata")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/metadata" "${_kritametadata_build_dir}")

target_include_directories(kritametadata
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/metadata>"
        "$<BUILD_INTERFACE:${_kritametadata_build_dir}>"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritametadata PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)

disable_seed_tests(libs/flake)
disable_seed_subdir(libs/flake/resources/tests)

set(KSeExpr_FOUND OFF)
set(_seed_raqm_source_dir "${KRITA_SOURCE_DIR}/3rdparty_vendor/raqm/src/raqm-0.10.1/src")
set(_seed_raqm_build_dir "${CMAKE_CURRENT_BINARY_DIR}/libraqm")
file(MAKE_DIRECTORY "${_seed_raqm_build_dir}")

set(RAQM_VERSION_MAJOR 0)
set(RAQM_VERSION_MINOR 10)
set(RAQM_VERSION_MICRO 1)
set(RAQM_VERSION "0.10.1.1")
configure_file(
    "${_seed_raqm_source_dir}/raqm-version.h.in"
    "${_seed_raqm_build_dir}/raqm-version.h")

add_library(libraqm STATIC
    "${_seed_raqm_source_dir}/raqm.c")
set_property(TARGET libraqm PROPERTY C_STANDARD 99)
target_include_directories(libraqm
    PUBLIC
        "${_seed_raqm_source_dir}"
        "${_seed_raqm_build_dir}")
target_link_libraries(libraqm
    PUBLIC
        Freetype::Freetype
    PRIVATE
        HarfBuzz::HarfBuzz
        FriBidi::FriBidi)
add_library(Raqm::Raqm ALIAS libraqm)

set(_kritaflake_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaflake")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/flake" "${_kritaflake_build_dir}")

target_include_directories(kritaflake
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake/commands>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake/tools>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake/svg>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake/text>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake/xsimd>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/flake/resources>"
        "$<BUILD_INTERFACE:${_kritaflake_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaflake PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritaflake PRIVATE
    -fno-operator-names
    -include QIcon)
target_link_libraries(kritaflake PRIVATE
    seed_text_dep_stubs)

set(_kritaresourcewidgets_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaresourcewidgets")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/resourcewidgets" "${_kritaresourcewidgets_build_dir}")

target_include_directories(kritaresourcewidgets
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/resourcewidgets>"
        "$<BUILD_INTERFACE:${_kritaresourcewidgets_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaresourcewidgets PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritaresourcewidgets PRIVATE
    -fno-operator-names)

disable_seed_tests(libs/widgets)

set(_kritawidgets_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritawidgets")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/widgets" "${_kritawidgets_build_dir}")

target_include_directories(kritawidgets
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/widgets>"
        "$<BUILD_INTERFACE:${_kritawidgets_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritawidgets PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritawidgets PRIVATE
    -fno-operator-names
    -include "${_external_include_dir}/seed_kritawidgets_forced_includes.h")

disable_seed_tests(libs/psdutils)

set(_kritapsdutils_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritapsdutils")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/psdutils" "${_kritapsdutils_build_dir}")

target_include_directories(kritapsdutils
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/psdutils>"
        "$<BUILD_INTERFACE:${_kritapsdutils_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritapsdutils PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritapsdutils PRIVATE
    -fno-operator-names)
target_link_libraries(kritapsdutils PRIVATE
    seed_text_dep_stubs)

disable_seed_tests(libs/image)
disable_seed_tests(libs/image/tiles3)

set(_kritaimage_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaimage")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/image" "${_kritaimage_build_dir}")

target_include_directories(kritaimage
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/brushengine>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/commands>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/commands_new>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/filter>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/floodfill>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/generator>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/layerstyles>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/processing>"
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/image/tiles3>"
        "$<BUILD_INTERFACE:${_kritaimage_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaimage PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritaimage PRIVATE
    -fno-operator-names
    -include QIcon)

disable_seed_tests(libs/brush)

set(_kritalibbrush_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritalibbrush")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/brush" "${_kritalibbrush_build_dir}")

target_include_directories(kritalibbrush
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/brush>"
        "$<BUILD_INTERFACE:${_kritalibbrush_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritalibbrush PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritalibbrush PRIVATE
    -fno-operator-names
    -include QIcon)

set(_kritaimpex_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaimpex")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/impex" "${_kritaimpex_build_dir}")

target_include_directories(kritaimpex
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/impex>"
        "$<BUILD_INTERFACE:${_kritaimpex_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaimpex PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritaimpex PRIVATE
    -fno-operator-names)

disable_seed_tests(libs/ui)

set(_kritaui_build_dir "${CMAKE_CURRENT_BINARY_DIR}/kritaui")
add_subdirectory("${KRITA_SOURCE_DIR}/libs/ui" "${_kritaui_build_dir}")

target_include_directories(kritaui
    PUBLIC
        "$<BUILD_INTERFACE:${KRITA_SOURCE_DIR}/libs/ui>"
        "$<BUILD_INTERFACE:${_kritaui_build_dir}>"
    PRIVATE
        "${KRITA_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_external_include_dir}")
target_compile_definitions(kritaui PRIVATE
    QT_NO_SIGNALS_SLOTS_KEYWORDS)
target_compile_options(kritaui PRIVATE
    -fno-operator-names
    -include "${_external_include_dir}/seed_kritaui_forced_includes.h")
