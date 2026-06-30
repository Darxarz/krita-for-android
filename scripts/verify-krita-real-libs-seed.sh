#!/usr/bin/env bash
set -euo pipefail

build_dir="$1"
abi="$2"
machine_pattern="$3"
readelf_bin="$4"

qt_core="libQt5Core_${abi}.so"
qt_gui="libQt5Gui_${abi}.so"
qt_network="libQt5Network_${abi}.so"
qt_widgets="libQt5Widgets_${abi}.so"
qt_xml="libQt5Xml_${abi}.so"
qt_svg="libQt5Svg_${abi}.so"
qt_concurrent="libQt5Concurrent_${abi}.so"
qt_sql="libQt5Sql_${abi}.so"
qt_printsupport="libQt5PrintSupport_${abi}.so"
qt_androidextras="libQt5AndroidExtras_${abi}.so"

declare -A library_names=(
    [version]="libkritaversion.so"
    [global]="libkritaglobal.so"
    [plugin]="libkritaplugin.so"
    [multiarch]="libkritamultiarch.so"
    [color]="libkritacolor.so"
    [store]="libkritastore.so"
    [resources]="libkritaresources.so"
    [widgetutils]="libkritawidgetutils.so"
    [command]="libkritacommand.so"
    [pigment]="libkritapigment.so"
    [metadata]="libkritametadata.so"
    [flake]="libkritaflake.so"
    [resourcewidgets]="libkritaresourcewidgets.so"
    [widgets]="libkritawidgets.so"
    [psdutils]="libkritapsdutils.so"
    [image]="libkritaimage.so"
    [brush]="libkritalibbrush.so"
    [impex]="libkritaimpex.so"
    [ui]="libkritaui.so"
    [libkis]="libkritalibkis.so"
)

ordered_libraries=(
    version global plugin multiarch color store resources widgetutils command
    pigment metadata flake resourcewidgets widgets psdutils image brush impex
    ui libkis
)

declare -A library_paths=()
for key in "${ordered_libraries[@]}"; do
    library_paths[$key]="$(find "$build_dir" -name "${library_names[$key]}" -print -quit)"
    test -n "${library_paths[$key]}"
    file "${library_paths[$key]}"
done

check_machine() {
    local key="$1"
    local output="$RUNNER_TEMP/krita-${key}-elf-header.txt"
    "$readelf_bin" -h "${library_paths[$key]}" | tee "$output"
    grep -Eq "$machine_pattern" "$output"
}

check_deps() {
    local key="$1"
    shift
    local output="$RUNNER_TEMP/krita-${key}-dynamic.txt"
    "$readelf_bin" -d "${library_paths[$key]}" | tee "$output"
    local dependency
    for dependency in "$@"; do
        grep -Fq "Shared library: [$dependency]" "$output"
    done
}

check_symbol() {
    local key="$1"
    local symbol="$2"
    local output="$RUNNER_TEMP/krita-${key}-symbols.txt"
    "$readelf_bin" -Ws "${library_paths[$key]}" | tee "$output"
    grep -q "$symbol" "$output"
}

for key in "${ordered_libraries[@]}"; do
    check_machine "$key"
done

check_deps version "$qt_core"
check_symbol version "_ZN19KritaVersionWrapper13versionString"

check_deps global \
    libkritaversion.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_widgets" \
    "$qt_xml" \
    "$qt_androidextras"
check_symbol global "_ZN14KisUsageLogger10initializeEv"

check_deps plugin libkritaglobal.so "$qt_core"
check_symbol plugin "KoPluginLoader"

check_deps multiarch libkritaglobal.so "$qt_core"
check_symbol multiarch "vectorizationConfiguration"

check_deps color libkritaglobal.so "$qt_core"
check_symbol color "KisColorManager"

check_deps store libkritaglobal.so "$qt_core"
check_symbol store "KoStore"

check_deps resources \
    libkritaglobal.so \
    libkritaplugin.so \
    libkritastore.so \
    "$qt_core" \
    "$qt_widgets" \
    "$qt_sql"
check_symbol resources "KisResourceLocator"

check_deps widgetutils \
    libkritaglobal.so \
    libkritaresources.so \
    "$qt_core" \
    "$qt_widgets" \
    "$qt_printsupport" \
    "$qt_androidextras"
check_symbol widgetutils "KisActionRegistry"

check_deps command libkritawidgetutils.so "$qt_core" "$qt_widgets"
check_symbol command "KUndo2Stack"

check_deps pigment \
    libkritaglobal.so \
    libkritaplugin.so \
    libkritastore.so \
    libkritaresources.so \
    libkritacommand.so \
    libkritamultiarch.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_xml"
check_symbol pigment "KoColorSpaceRegistry"

check_deps metadata \
    libkritaglobal.so \
    libkritaplugin.so \
    libkritawidgetutils.so \
    "$qt_core"
check_symbol metadata "KisMetaData"

check_deps flake \
    libkritapigment.so \
    libkritawidgetutils.so \
    libkritacommand.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_widgets" \
    "$qt_svg" \
    "$qt_xml"
check_symbol flake "KoShapeRegistry"

check_deps resourcewidgets \
    libkritaglobal.so \
    libkritaplugin.so \
    libkritastore.so \
    libkritaresources.so \
    libkritawidgetutils.so \
    "$qt_core" \
    "$qt_widgets" \
    "$qt_sql"
check_symbol resourcewidgets "KisResourceItemChooser"

check_deps widgets \
    libkritaglobal.so \
    libkritaflake.so \
    libkritapigment.so \
    libkritawidgetutils.so \
    libkritaresources.so \
    libkritaresourcewidgets.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_widgets" \
    "$qt_printsupport"
check_symbol widgets "KoZoomAction"

check_deps psdutils \
    libkritapigment.so \
    libkritaglobal.so \
    libkritaflake.so \
    "$qt_core" \
    "$qt_gui"
check_symbol psdutils "KisAslReader"

check_deps image \
    libkritawidgets.so \
    libkritapsdutils.so \
    libkritapigment.so \
    libkritamultiarch.so \
    "$qt_core" \
    "$qt_gui"
check_symbol image "KisImage"

check_deps brush \
    libkritaimage.so \
    libkritamultiarch.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_widgets" \
    "$qt_svg"
check_symbol brush "KisBrushRegistry"

check_deps impex libkritaimage.so "$qt_core"
check_symbol impex "KisExportCheckRegistry"

check_deps ui \
    libkritaimpex.so \
    libkritaimage.so \
    libkritalibbrush.so \
    libkritawidgets.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_network" \
    "$qt_widgets" \
    "$qt_concurrent" \
    "$qt_androidextras"
check_symbol ui "KisPart"

check_deps libkis \
    libkritaui.so \
    libkritaimage.so \
    libkritaversion.so \
    "$qt_core" \
    "$qt_gui" \
    "$qt_widgets"
check_symbol libkis "Krita"
