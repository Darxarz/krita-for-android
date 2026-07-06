#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 8 ]; then
    echo "usage: $0 <abi> <triplet> <pyqt-prefix> <real-seed-dir> <krita-source-dir> <output-dir> <readelf> <machine-pattern>" >&2
    exit 2
fi

abi="$1"
triplet="$2"
pyqt_prefix="$3"
real_seed_dir="$4"
krita_source_dir="$5"
output_dir="$6"
readelf_bin="$7"
machine_pattern="$8"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
pykrita_stub="$repo_root/probes/krita-python-runtime-payload/pykrita.py"
safe_import_probe="$repo_root/probes/krita-python-runtime-payload/krita_probe_safe_import.py"

python_version="3.14"
site_packages="$pyqt_prefix/lib/python${python_version}/site-packages"

test -f "$pyqt_prefix/lib/libpython${python_version}.so"
test -f "$pyqt_prefix/lib/libQt5Core_${abi}.so"
test -f "$site_packages/PyQt5/sip.cpython-314-${triplet}.so"
test -f "$pykrita_stub"
test -f "$safe_import_probe"
test -d "$krita_source_dir/plugins/extensions/pykrita/plugin/krita"

pykrita_module="$(find "$real_seed_dir" -name "krita.so" -print -quit)"
kritapykrita_module="$(find "$real_seed_dir" -name "libkritapykrita.so" -print -quit)"
test -f "$pykrita_module"
test -f "$kritapykrita_module"

mkdir -p "$output_dir"
rm -rf "$output_dir/jniLibs" "$output_dir/assets" "$output_dir/MANIFEST.txt"

jni_lib_dir="$output_dir/jniLibs/$abi"
assets_python_dir="$output_dir/assets/python"
python_lib_dir="$assets_python_dir/lib"
krita_python_libs_dir="$assets_python_dir/krita-python-libs"

mkdir -p "$jni_lib_dir" "$python_lib_dir" "$krita_python_libs_dir/PyKrita"

find "$pyqt_prefix/lib" -maxdepth 1 -type f \( -name "*.so" -o -name "*.so.*" \) \
    -exec cp -a {} "$jni_lib_dir/" \;

find "$real_seed_dir" -type f -name "libkrita*.so" \
    -exec cp -a {} "$jni_lib_dir/" \;

cp -a "$pyqt_prefix/lib/python${python_version}" "$python_lib_dir/"
cp -a "$pykrita_module" "$krita_python_libs_dir/PyKrita/krita.so"
cp -a "$pykrita_stub" "$krita_python_libs_dir/pykrita.py"
cp -a "$safe_import_probe" "$krita_python_libs_dir/krita_probe_safe_import.py"
cp -a "$krita_source_dir/plugins/extensions/pykrita/plugin/krita" "$krita_python_libs_dir/"

test -f "$assets_python_dir/lib/python${python_version}/os.py"
test -f "$assets_python_dir/lib/python${python_version}/site.py"
test -f "$assets_python_dir/lib/python${python_version}/site-packages/PyQt5/sip.cpython-314-${triplet}.so"
test -f "$krita_python_libs_dir/PyKrita/krita.so"
test -f "$krita_python_libs_dir/pykrita.py"
test -f "$krita_python_libs_dir/krita_probe_safe_import.py"
test -f "$krita_python_libs_dir/krita/__init__.py"
test -f "$jni_lib_dir/libpython${python_version}.so"
test -f "$jni_lib_dir/libQt5Core_${abi}.so"
test -f "$jni_lib_dir/libkritalibkis.so"
test -f "$jni_lib_dir/libkritapykrita.so"

check_machine() {
    local path="$1"
    local name
    name="$(basename "$path")"
    local header="$output_dir/${name}.elf-header.txt"
    "$readelf_bin" -h "$path" | tee "$header"
    grep -Eq "$machine_pattern" "$header"
    rm -f "$header"
}

check_machine "$jni_lib_dir/libpython${python_version}.so"
check_machine "$jni_lib_dir/libQt5Core_${abi}.so"
check_machine "$jni_lib_dir/libkritalibkis.so"
check_machine "$jni_lib_dir/libkritapykrita.so"
check_machine "$krita_python_libs_dir/PyKrita/krita.so"
check_machine "$assets_python_dir/lib/python${python_version}/site-packages/PyQt5/sip.cpython-314-${triplet}.so"

{
    echo "Krita Android Python runtime payload"
    echo "abi=$abi"
    echo "triplet=$triplet"
    echo
    find "$output_dir" -type f \
        | sed "s#^$output_dir/##" \
        | sort
} > "$output_dir/MANIFEST.txt"

echo "Staged Krita Android Python runtime payload at $output_dir"
