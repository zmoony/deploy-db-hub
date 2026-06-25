param(
    [string]$QtRoot = "D:\systemInsall\Qt",
    [string]$QtVersion = "6.11.1",
    [string]$QtKit = "mingw_64"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$QtPrefix = Join-Path $QtRoot "$QtVersion\$QtKit"
$WindeployQt = Join-Path $QtPrefix "bin\windeployqt.exe"
$Strip = Join-Path $QtRoot "Tools\mingw1310_64\bin\strip.exe"
$MingwBin = Join-Path $QtRoot "Tools\mingw1310_64\bin"
$BuildDir = Join-Path $ProjectRoot "build-release"
$DistDir = Join-Path $ProjectRoot "dist\windows"
$ExeSource = Join-Path $BuildDir "deploy-hub.exe"
$ExeTarget = Join-Path $DistDir "deploy-hub.exe"
function Resolve-FluentUiSourceDirectory {
    param([Parameter(Mandatory = $true)][string]$BuildDir)

    $candidates = @(
        (Join-Path $BuildDir "qml\FluentUI"),
        (Join-Path $BuildDir "FluentUI")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate "qmldir")) {
            return $candidate
        }
    }
    return $null
}

function Install-FluentUiPlugin {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDir,
        [Parameter(Mandatory = $true)][string]$DistDir
    )

    $qmlRoot = Join-Path $DistDir "qml"
    $FluentUiTarget = Join-Path $qmlRoot "FluentUI"
    New-Item -ItemType Directory -Force -Path $qmlRoot | Out-Null
    if (Test-Path -LiteralPath $FluentUiTarget) {
        Remove-Item -LiteralPath $FluentUiTarget -Recurse -Force | Out-Null
    }
    Copy-Item -Path $SourceDir -Destination $qmlRoot -Recurse -Force | Out-Null

    if (-not (Test-Path -LiteralPath (Join-Path $FluentUiTarget "qmldir"))) {
        throw "FluentUI plugin copy failed: qmldir missing in $FluentUiTarget"
    }
    Write-Host "Copied FluentUI plugin: $SourceDir -> $FluentUiTarget"
    $dllPath = Join-Path $FluentUiTarget "fluentuiplugin.dll"
    return $dllPath
}

function Stop-DeployHubProcesses {
    $processes = Get-Process -Name "deploy-hub" -ErrorAction SilentlyContinue
    if ($null -eq $processes) {
        return
    }
    Write-Host "Stopping running deploy-hub.exe ..."
    $processes | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
}

function Clear-DistributionDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    Stop-DeployHubProcesses

    for ($attempt = 1; $attempt -le 3; $attempt++) {
        try {
            Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
            return
        } catch {
            if ($attempt -lt 3) {
                Write-Host "Retrying cleanup ($attempt/3): $($_.Exception.Message)"
                Stop-DeployHubProcesses
                Start-Sleep -Seconds 2
                continue
            }

            $backupPath = "$Path.old-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
            Write-Host "Cleanup blocked by locked files. Moving old package to: $backupPath"
            Move-Item -LiteralPath $Path -Destination $backupPath -Force -ErrorAction Stop
        }
    }
}

function Remove-IfExists {
    param([Parameter(Mandatory = $true)][string]$Path)
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "Removed: $Path"
    }
}

function Trim-DistributionDirectory {
    param([Parameter(Mandatory = $true)][string]$Path)

    Remove-IfExists (Join-Path $Path "qmltooling")
    Remove-IfExists (Join-Path $Path "translations")

    $qmlRoot = Join-Path $Path "qml"
    if (-not (Test-Path -LiteralPath $qmlRoot)) {
        return
    }

    $keepQmlModules = @(
        "QtQuick",
        "QtQml",
        "FluentUI",
        "Qt5Compat"
    )
    Get-ChildItem -LiteralPath $qmlRoot -Directory | ForEach-Object {
        if ($keepQmlModules -notcontains $_.Name) {
            Remove-IfExists $_.FullName
        }
    }

    $quickRoot = Join-Path $qmlRoot "QtQuick"
    if (Test-Path -LiteralPath $quickRoot) {
        $keepQuickModules = @(
            "Controls",
            "Layouts",
            "Effects",
            "Templates",
            "Window",
            "Dialogs",
            "NativeStyle",
            "Particles",
            "Shapes"
        )
        Get-ChildItem -LiteralPath $quickRoot -Directory | ForEach-Object {
            if ($keepQuickModules -notcontains $_.Name) {
                Remove-IfExists $_.FullName
            }
        }
    }
}

Clear-DistributionDirectory -Path $DistDir
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

Copy-Item -LiteralPath $ExeSource -Destination $ExeTarget -Force

if (Test-Path -LiteralPath $Strip) {
    & $Strip --strip-all $ExeTarget
    Write-Host "Stripped: $ExeTarget"
}

$ToolsSource = Join-Path $BuildDir "tools"
$ToolsTarget = Join-Path $DistDir "tools"
if (Test-Path -LiteralPath $ToolsSource) {
    Copy-Item -LiteralPath $ToolsSource -Destination $ToolsTarget -Recurse -Force
}

$ImagesSource = Join-Path $ProjectRoot "images"
$ImagesTarget = Join-Path $DistDir "images"
if (Test-Path -LiteralPath $ImagesSource) {
    Copy-Item -LiteralPath $ImagesSource -Destination $ImagesTarget -Recurse -Force
}

$env:Path = "$MingwBin;$(Join-Path $QtPrefix 'bin');$env:Path"

$QmlDir = Join-Path $ProjectRoot "src\qml\DeployHub"
$FluentUiSource = Resolve-FluentUiSourceDirectory -BuildDir $BuildDir

$WindeployArgs = @(
    "--release",
    "--compiler-runtime",
    "--no-translations",
    "--qmldir", $QmlDir,
    "--dir", $DistDir
)
if ($null -ne $FluentUiSource) {
    $WindeployArgs += @("--qmlimport", $FluentUiSource)
}
$WindeployArgs += $ExeTarget

& $WindeployQt @WindeployArgs

if ($null -ne $FluentUiSource) {
    $fluentDll = Install-FluentUiPlugin -SourceDir $FluentUiSource -DistDir $DistDir
    if (Test-Path -LiteralPath $fluentDll) {
        & $WindeployQt --release --no-translations --compiler-runtime --dir $DistDir $fluentDll
        Write-Host "Deployed FluentUI plugin dependencies"
    }
} else {
    Write-Warning "FluentUI plugin not found under build-release. Rebuild with: scripts\build-release.ps1 -UseFluentUI"
}

Trim-DistributionDirectory -Path $DistDir

$exeSizeMb = [math]::Round((Get-Item -LiteralPath $ExeTarget).Length / 1MB, 1)
$distSizeMb = [math]::Round(((Get-ChildItem -LiteralPath $DistDir -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB), 1)
Write-Host "Packaged: $ExeTarget ($exeSizeMb MB), dist total ~$distSizeMb MB"
