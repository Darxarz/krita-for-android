[CmdletBinding()]
param(
    [ValidateSet("arm64-v8a", "x86_64")]
    [string] $Abi = "arm64-v8a",

    [string] $PythonVersion = "3.14.0",

    [string] $OutputDir
)

$ErrorActionPreference = "Stop"

$scriptRoot = if ($PSScriptRoot) {
    $PSScriptRoot
} else {
    Split-Path -Parent $MyInvocation.MyCommand.Path
}
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

if (-not $OutputDir) {
    $OutputDir = Join-Path $repoRoot "artifacts/python-android"
}

$triplets = @{
    "arm64-v8a" = "aarch64-linux-android"
    "x86_64" = "x86_64-linux-android"
}

$hashes = @{
    "3.14.0:aarch64-linux-android" = "F09BD8AE86F408580881AE224E848805C267BB65B3111DC77B41BDA79456DA6C"
    "3.14.0:x86_64-linux-android" = "5C953DF43E47C43CE55888ACC5F09F6CAC67F2E292480B906BE37C14381516E7"
}

if (-not $triplets.ContainsKey($Abi)) {
    throw "Unsupported Android ABI: $Abi"
}

$triplet = $triplets[$Abi]
$hashKey = "$PythonVersion`:$triplet"
if (-not $hashes.ContainsKey($hashKey)) {
    throw "No SHA256 is pinned for Python $PythonVersion / $triplet"
}

$downloadDir = Join-Path $repoRoot "artifacts/downloads"
$dest = Join-Path $OutputDir $Abi
$prefix = Join-Path $dest "prefix"
$archiveName = "python-$PythonVersion-$triplet.tar.gz"
$archive = Join-Path $downloadDir $archiveName
$url = "https://www.python.org/ftp/python/$PythonVersion/$archiveName"

New-Item -ItemType Directory -Force -Path $downloadDir | Out-Null
New-Item -ItemType Directory -Force -Path $dest | Out-Null

if (-not (Test-Path -LiteralPath $archive)) {
    Write-Host "Downloading $url"
    Invoke-WebRequest -Uri $url -OutFile $archive
}

$actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToUpperInvariant()
$expectedHash = $hashes[$hashKey].ToUpperInvariant()
if ($actualHash -ne $expectedHash) {
    throw "SHA256 mismatch for $archiveName. Expected $expectedHash, got $actualHash"
}

if (-not (Test-Path -LiteralPath $prefix)) {
    Write-Host "Extracting $archiveName to $dest"
    tar -xzf $archive -C $dest ./prefix
}

if (-not (Test-Path -LiteralPath (Join-Path $prefix "lib/libpython3.14.so"))) {
    throw "Extracted package does not contain lib/libpython3.14.so"
}

Write-Host "Python Android prefix: $prefix"
Write-Output $prefix
