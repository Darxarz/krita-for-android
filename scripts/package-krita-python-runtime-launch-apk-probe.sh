#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 10 ]; then
    echo "usage: $0 <abi> <payload-dir> <init-probe-so> <launcher-so> <java-source-dir> <output-dir> <android-jar> <build-tools-dir> <readelf> <machine-pattern>" >&2
    exit 2
fi

abi="$1"
payload_dir="$2"
init_probe_so="$3"
launcher_so="$4"
java_source_dir="$5"
output_dir="$6"
android_jar="$7"
build_tools_dir="$8"
readelf_bin="$9"
machine_pattern="${10}"

test -d "$payload_dir/jniLibs/$abi"
test -d "$payload_dir/assets/python"
test -f "$init_probe_so"
test -f "$launcher_so"
test -f "$java_source_dir/org/krita/android/pythonruntimeprobe/MainActivity.java"
test -f "$android_jar"
test -x "$build_tools_dir/aapt2"
test -x "$build_tools_dir/d8"
test -x "$build_tools_dir/zipalign"
test -x "$build_tools_dir/apksigner"

rm -rf "$output_dir"
mkdir -p "$output_dir"

work_dir="$output_dir/work"
assets_dir="$work_dir/assets"
native_lib_dir="$work_dir/lib/$abi"
classes_dir="$work_dir/classes"
dex_dir="$work_dir/dex"
mkdir -p "$assets_dir" "$native_lib_dir" "$classes_dir" "$dex_dir"

cp -a "$payload_dir/assets/python" "$assets_dir/"
find "$payload_dir/jniLibs/$abi" -maxdepth 1 -type f \( -name "*.so" -o -name "*.so.*" \) \
    -exec cp -a {} "$native_lib_dir/" \;
cp -a "$init_probe_so" "$native_lib_dir/libkrita_python_runtime_init_probe.so"
cp -a "$launcher_so" "$native_lib_dir/libkrita_python_runtime_launcher.so"

test -f "$assets_dir/python/lib/python3.14/os.py"
test -f "$assets_dir/python/krita-python-libs/PyKrita/krita.so"
test -f "$native_lib_dir/libpython3.14.so"
test -f "$native_lib_dir/libkritapykrita.so"
test -f "$native_lib_dir/libkrita_python_runtime_init_probe.so"
test -f "$native_lib_dir/libkrita_python_runtime_launcher.so"

javac -encoding UTF-8 \
    -source 8 \
    -target 8 \
    -bootclasspath "$android_jar" \
    -d "$classes_dir" \
    $(find "$java_source_dir" -type f -name "*.java" | sort)

"$build_tools_dir/d8" \
    --min-api 24 \
    --output "$dex_dir" \
    $(find "$classes_dir" -type f -name "*.class" | sort)

test -f "$dex_dir/classes.dex"
cp "$dex_dir/classes.dex" "$work_dir/classes.dex"

cat > "$work_dir/AndroidManifest.xml" <<'EOF'
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="org.krita.android.pythonruntimeprobe">
    <uses-sdk
        android:minSdkVersion="24"
        android:targetSdkVersion="35" />
    <application
        android:extractNativeLibs="true"
        android:label="Krita Python Runtime Probe"
        android:theme="@android:style/Theme.Material.Light">
        <activity
            android:name=".MainActivity"
            android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
EOF

unsigned_base_apk="$output_dir/krita-python-runtime-launch-probe-${abi}-base.apk"
unsigned_apk="$output_dir/krita-python-runtime-launch-probe-${abi}-unsigned.apk"
aligned_apk="$output_dir/krita-python-runtime-launch-probe-${abi}-aligned.apk"
signed_apk="$output_dir/krita-python-runtime-launch-probe-${abi}.apk"
keystore="$output_dir/debug.keystore"

"$build_tools_dir/aapt2" link \
    --manifest "$work_dir/AndroidManifest.xml" \
    -I "$android_jar" \
    -A "$assets_dir" \
    -o "$unsigned_base_apk"

cp "$unsigned_base_apk" "$unsigned_apk"
(
    cd "$work_dir"
    zip -qr "$unsigned_apk" lib classes.dex
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
require_entry "classes.dex"
require_entry "assets/python/lib/python3.14/os.py"
require_entry "assets/python/krita-python-libs/PyKrita/krita.so"
require_entry "lib/$abi/libpython3.14.so"
require_entry "lib/$abi/libkritapykrita.so"
require_entry "lib/$abi/libkrita_python_runtime_init_probe.so"
require_entry "lib/$abi/libkrita_python_runtime_launcher.so"

check_machine() {
    local apk_entry="$1"
    local extracted="$output_dir/$(basename "$apk_entry")"
    unzip -p "$signed_apk" "$apk_entry" > "$extracted"
    "$readelf_bin" -h "$extracted" | tee "$extracted.elf-header.txt"
    grep -Eq "$machine_pattern" "$extracted.elf-header.txt"
    rm -f "$extracted" "$extracted.elf-header.txt"
}

check_dynamic_needed() {
    local apk_entry="$1"
    local needed="$2"
    local extracted="$output_dir/$(basename "$apk_entry")"
    unzip -p "$signed_apk" "$apk_entry" > "$extracted"
    "$readelf_bin" -d "$extracted" | tee "$extracted.dynamic.txt"
    grep -Fq "Shared library: [$needed]" "$extracted.dynamic.txt"
    rm -f "$extracted" "$extracted.dynamic.txt"
}

check_machine "lib/$abi/libpython3.14.so"
check_machine "lib/$abi/libkritapykrita.so"
check_machine "lib/$abi/libkrita_python_runtime_init_probe.so"
check_machine "lib/$abi/libkrita_python_runtime_launcher.so"
check_machine "assets/python/krita-python-libs/PyKrita/krita.so"
check_dynamic_needed "lib/$abi/libkrita_python_runtime_launcher.so" "libkrita_python_runtime_init_probe.so"

{
    echo "Krita Android Python runtime launch APK probe"
    echo "abi=$abi"
    echo
    cat "$output_dir/apk-entries.txt"
} > "$output_dir/MANIFEST.txt"

echo "Packaged launchable Krita Android Python runtime APK probe at $signed_apk"
