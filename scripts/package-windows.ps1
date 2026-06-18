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
            try {
                Move-Item -LiteralPath $Path -Destination $backupPath -Force -ErrorAction Stop
            } catch {
                throw @"
无法清理旧的打包目录，文件仍被占用（常见原因：deploy-hub.exe 正在运行）。
请先关闭 dist\windows 下的 deploy-hub.exe，或在任务管理器中结束 deploy-hub 进程，然后重新运行 complie-windosw.bat。
原始错误：$($_.Exception.Message)
"@
            }
        }
    }
}

Clear-DistributionDirectory -Path $DistDir
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
