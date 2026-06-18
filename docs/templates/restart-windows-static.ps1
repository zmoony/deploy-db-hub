# Deploy Hub restart template: static files + IIS (symlink/junction current model)
# Install: copy to <remoteBaseDir>\restart.ps1
#
# Deploy Hub uploads to releases\<version>\ then switches current junction to that directory.
# restart only recycles IIS (optional); no robocopy (VersionDir == CurrentDir).

param(
    [string]$App,
    [string]$VersionDir,
    [string]$CurrentDir,
    [string[]]$Artifact
)

# --- CONFIG (edit before use) ---
$RecycleAppPool = $true
$AppPoolName    = "DefaultAppPool"
# --- END CONFIG ---

if (-not $VersionDir -or -not $CurrentDir) {
    Write-Error "missing VersionDir or CurrentDir"
    exit 2
}

if (-not (Test-Path -LiteralPath $CurrentDir -PathType Container)) {
    Write-Error "current dir missing: $CurrentDir"
    exit 2
}

if ($RecycleAppPool) {
    try {
        Import-Module WebAdministration -ErrorAction Stop
        Restart-WebAppPool -Name $AppPoolName
    } catch {
        Write-Error "IIS app pool recycle failed: $AppPoolName - $_"
        exit 1
    }
}

Write-Output "static deploy ok: app=$App (current serves releases content)"
exit 0
