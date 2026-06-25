[CmdletBinding()]
param(
    [ValidateSet("arm64-v8a", "x86_64")]
    [string] $Abi = "arm64-v8a",

    [string] $PythonPrefix,

    [string] $AndroidNdkRoot = $env:ANDROID_NDK_ROOT,

    [string] $AndroidPlatform = "android-24",

    [string] $BuildDir
)

$ErrorActionPreference = "Stop"

$scriptRoot = if ($PSScriptRoot) {
    $PSScriptRoot
} else {
    Split-Path -Parent $MyInvocation.MyCommand.Path
}
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

if (-not $BuildDir) {
    $BuildDir = Join-Path $repoRoot "build/python-embed-probe"
}

if (-not $PythonPrefix) {
    $PythonPrefix = & (Join-Path $scriptRoot "download-python-android.ps1") -Abi $Abi
}
$PythonPrefix = Resolve-Path $PythonPrefix

if (-not $AndroidNdkRoot) {
    $candidates = @(
        $env:ANDROID_NDK_HOME,
        $(if ($env:ANDROID_HOME) { Join-Path $env:ANDROID_HOME "ndk/27.3.13750724" }),
        $(if ($env:ANDROID_SDK_ROOT) { Join-Path $env:ANDROID_SDK_ROOT "ndk/27.3.13750724" })
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    if ($candidates.Count -gt 0) {
        $AndroidNdkRoot = $candidates[0]
    }
}

if (-not $AndroidNdkRoot) {
    throw "Android NDK root was not provided. Pass -AndroidNdkRoot or set ANDROID_NDK_ROOT."
}

$toolchain = Join-Path $AndroidNdkRoot "build/cmake/android.toolchain.cmake"
if (-not (Test-Path -LiteralPath $toolchain)) {
    throw "Android CMake toolchain file was not found: $toolchain"
}

$sourceDir = Join-Path $repoRoot "probes/cpython-android-embed"
$abiBuildDir = Join-Path $BuildDir $Abi
New-Item -ItemType Directory -Force -Path $abiBuildDir | Out-Null

$generatorArgs = @()
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    $generatorArgs = @("-G", "Ninja")
}

cmake `
    -S $sourceDir `
    -B $abiBuildDir `
    @generatorArgs `
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
    "-DANDROID_ABI=$Abi" `
    "-DANDROID_PLATFORM=$AndroidPlatform" `
    "-DPYTHON_ANDROID_PREFIX=$PythonPrefix"

cmake --build $abiBuildDir --verbose

Write-Host "Built probe in $abiBuildDir"
