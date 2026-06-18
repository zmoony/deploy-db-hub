param(
    [string]$QtRoot = "D:\systemInsall\Qt",
    [string]$QtVersion = "6.11.1",
    [string]$QtKit = "mingw_64"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$QtPrefix = Join-Path $QtRoot "$QtVersion\$QtKit"
$WindeployQt = Join-Path $QtPrefix "bin\windeployqt.exe"
$MingwBin = Join-Path $QtRoot "Tools\mingw1310_64\bin"
$BuildDir = Join-Path $ProjectRoot "build-release"
$DistDir = Join-Path $ProjectRoot "dist\windows"
$ExeSource = Join-Path $BuildDir "deploy-hub.exe"
$ExeTarget = Join-Path $DistDir "deploy-hub.exe"

if (-not (Test-Path -LiteralPath $WindeployQt)) {
    throw "windeployqt.exe not found: $WindeployQt"
}
if (-not (Test-Path -LiteralPath $ExeSource)) {
    throw "Executable not found. Run scripts\build-release.ps1 first: $ExeSource"
}

if (Test-Path -LiteralPath $DistDir) {
    Remove-Item -LiteralPath $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

Copy-Item -LiteralPath $ExeSource -Destination $ExeTarget -Force

$ImagesSource = Join-Path $ProjectRoot "images"
$ImagesTarget = Join-Path $DistDir "images"
if (Test-Path -LiteralPath $ImagesSource) {
    Copy-Item -LiteralPath $ImagesSource -Destination $ImagesTarget -Recurse -Force
}

$env:Path = "$MingwBin;$(Join-Path $QtPrefix 'bin');$env:Path"

& $WindeployQt --release --compiler-runtime --dir $DistDir $ExeTarget

Write-Host "Packaged: $ExeTarget"
