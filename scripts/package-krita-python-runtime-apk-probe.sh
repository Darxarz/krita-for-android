#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 8 ]; then
    echo "usage: $0 <abi> <payload-dir> <init-probe-so> <output-dir> <android-jar> <build-tools-dir> <readelf> <machine-pattern>" >&2
    exit 2
fi

abi="$1"
payload_dir="$2"
init_probe_so="$3"
output_dir="$4"
android_jar="$5"
build_tools_dir="$6"
readelf_bin="$7"
machine_pattern="$8"

test -d "$payload_dir/jniLibs/$abi"
test -d "$payload_dir/assets/python"
test -f "$init_probe_so"
test -f "$android_jar"
test -x "$build_tools_dir/aapt2"
test -x "$build_tools_dir/zipalign"
test -x "$build_tools_dir/apksigner"

rm -rf "$output_dir"
mkdir -p "$output_dir"

work_dir="$output_dir/work"
assets_dir="$work_dir/assets"
native_lib_dir="$work_dir/lib/$abi"
mkdir -p "$assets_dir" "$native_lib_dir"

cp -a "$payload_dir/assets/python" "$assets_dir/"
find "$payload_dir/jniLibs/$abi" -maxdepth 1 -type f \( -name "*.so" -o -name "*.so.*" \) \
    -exec cp -a {} "$native_lib_dir/" \;
cp -a "$init_probe_so" "$native_lib_dir/libkrita_python_runtime_init_probe.so"

test -f "$assets_dir/python/lib/python3.14/os.py"
test -f "$assets_dir/python/lib/python3.14/site.py"
test -f "$assets_dir/python/krita-python-libs/PyKrita/krita.so"
test -f "$assets_dir/python/krita-python-libs/krita/__init__.py"
test -f "$native_lib_dir/libpython3.14.so"
test -f "$native_lib_dir/libkritalibkis.so"
test -f "$native_lib_dir/libkritapykrita.so"
test -f "$native_lib_dir/libkrita_python_runtime_init_probe.so"

cat > "$work_dir/AndroidManifest.xml" <<'EOF'
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="org.krita.android.pythonruntimeprobe">
    <uses-sdk
        android:minSdkVersion="24"
        android:targetSdkVersion="35" />
    <application
        android:extractNativeLibs="true"
        android:hasCode="false"
        android:label="Krita Python Runtime Probe" />
</manifest>
EOF

unsigned_base_apk="$output_dir/krita-python-runtime-probe-${abi}-base.apk"
unsigned_apk="$output_dir/krita-python-runtime-probe-${abi}-unsigned.apk"
aligned_apk="$output_dir/krita-python-runtime-probe-${abi}-aligned.apk"
signed_apk="$output_dir/krita-python-runtime-probe-${abi}.apk"
keystore="$output_dir/debug.keystore"

"$build_tools_dir/aapt2" link \
    --manifest "$work_dir/AndroidManifest.xml" \
    -I "$android_jar" \
    -A "$assets_dir" \
    -o "$unsigned_base_apk"

cp "$unsigned_base_apk" "$unsigned_apk"
(
    cd "$work_dir"
    zip -qr "$unsigned_apk" lib
)

"$build_tools_dir/zipalign" -f 4 "$unsigned_apk" "$aligned_apk"

keytool -genkeypair \
    -keystore "$keystore" \
    -storepass android \
    -keypass android \
    -alias androiddebugkey \
    -keyalg RSA \
    -keysize 2048 \
    -validity 10000 \
    -dname "CN=Android Debug,O=Krita Python Runtime Probe,C=US" >/dev/null

"$build_tools_dir/apksigner" sign \
    --ks "$keystore" \
    --ks-pass pass:android \
    --key-pass pass:android \
    --out "$signed_apk" \
    "$aligned_apk"

"$build_tools_dir/apksigner" verify --verbose --print-certs "$signed_apk"

zipinfo -1 "$signed_apk" | tee "$output_dir/apk-entries.txt"

require_entry() {
    local entry="$1"
    grep -Fxq "$entry" "$output_dir/apk-entries.txt"
}

require_entry "AndroidManifest.xml"
require_entry "assets/python/lib/python3.14/os.py"
require_entry "assets/python/lib/python3.14/site.py"
require_entry "assets/python/krita-python-libs/PyKrita/krita.so"
require_entry "assets/python/krita-python-libs/krita/__init__.py"
require_entry "lib/$abi/libpython3.14.so"
require_entry "lib/$abi/libkritalibkis.so"
require_entry "lib/$abi/libkritapykrita.so"
require_entry "lib/$abi/libkrita_python_runtime_init_probe.so"

check_machine() {
    local apk_entry="$1"
    local extracted="$output_dir/$(basename "$apk_entry")"
    unzip -p "$signed_apk" "$apk_entry" > "$extracted"
    "$readelf_bin" -h "$extracted" | tee "$extracted.elf-header.txt"
    grep -Eq "$machine_pattern" "$extracted.elf-header.txt"
    rm -f "$extracted" "$extracted.elf-header.txt"
}

check_machine "lib/$abi/libpython3.14.so"
check_machine "lib/$abi/libkritalibkis.so"
check_machine "lib/$abi/libkritapykrita.so"
check_machine "lib/$abi/libkrita_python_runtime_init_probe.so"
check_machine "assets/python/krita-python-libs/PyKrita/krita.so"

{
    echo "Krita Android Python runtime APK probe"
    echo "abi=$abi"
    echo
    cat "$output_dir/apk-entries.txt"
} > "$output_dir/MANIFEST.txt"

echo "Packaged Krita Android Python runtime APK probe at $signed_apk"
