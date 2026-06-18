param(
    [string]$QtRoot = "D:\systemInsall\Qt",
    [string]$QtVersion = "6.11.1",
    [string]$QtKit = "mingw_64"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$QtPrefix = Join-Path $QtRoot "$QtVersion\$QtKit"
$CMake = Join-Path $QtRoot "Tools\CMake_64\bin\cmake.exe"
$Ninja = Join-Path $QtRoot "Tools\Ninja\ninja.exe"
$MingwBin = Join-Path $QtRoot "Tools\mingw1310_64\bin"
$BuildDir = Join-Path $ProjectRoot "build-release"

if (-not (Test-Path -LiteralPath $QtPrefix)) {
    throw "Qt prefix not found: $QtPrefix"
}
if (-not (Test-Path -LiteralPath $CMake)) {
    throw "cmake.exe not found: $CMake"
}
if (-not (Test-Path -LiteralPath $Ninja)) {
    throw "ninja.exe not found: $Ninja"
}
if (-not (Test-Path -LiteralPath (Join-Path $MingwBin "g++.exe"))) {
    throw "g++.exe not found in: $MingwBin"
}

$env:Path = "$MingwBin;$(Join-Path $QtPrefix 'bin');$(Split-Path -Parent $CMake);$(Split-Path -Parent $Ninja);$env:Path"

& $CMake -S $ProjectRoot -B $BuildDir -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_MAKE_PROGRAM="$Ninja" `
    -DCMAKE_C_COMPILER="$(Join-Path $MingwBin 'gcc.exe')" `
    -DCMAKE_CXX_COMPILER="$(Join-Path $MingwBin 'g++.exe')" `
    -DCMAKE_PREFIX_PATH="$QtPrefix"

& $CMake --build $BuildDir --config Release
