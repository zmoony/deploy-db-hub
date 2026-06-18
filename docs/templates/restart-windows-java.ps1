# Deploy Hub restart template: Java jar + Windows Service
# Install: copy to <remoteBaseDir>\restart.ps1
# Exit codes: 0=ok 1=start failed 2=config error 3=health check failed

param(
    [string]$App,
    [string]$VersionDir,
    [string]$CurrentDir,
    [string[]]$Artifact
)

# --- CONFIG (edit before use) ---
$ServiceName = "DemoService"
$JarName     = "app.jar"
$HealthUrl   = ""   # optional, e.g. http://127.0.0.1:8080/health
$HealthTimeoutSec = 30
# --- END CONFIG ---

if (-not $VersionDir -or -not $CurrentDir -or -not $Artifact -or $Artifact.Count -eq 0) {
    Write-Error "missing required parameters"
    exit 2
}

$src = $Artifact[0]
if (-not (Test-Path -LiteralPath $src -PathType Leaf)) {
    Write-Error "artifact not found: $src"
    exit 2
}

New-Item -ItemType Directory -Force -Path $CurrentDir | Out-Null
Copy-Item -LiteralPath $src -Destination (Join-Path $CurrentDir $JarName) -Force

try {
    Restart-Service -Name $ServiceName -ErrorAction Stop
} catch {
    Write-Error "Restart-Service failed: $ServiceName - $_"
    exit 1
}

if ($HealthUrl) {
    $deadline = (Get-Date).AddSeconds($HealthTimeoutSec)
    while ((Get-Date) -lt $deadline) {
        try {
            $r = Invoke-WebRequest -Uri $HealthUrl -UseBasicParsing -TimeoutSec 5
            if ($r.StatusCode -ge 200 -and $r.StatusCode -lt 300) { break }
        } catch { Start-Sleep -Seconds 2 }
        if ((Get-Date) -ge $deadline) {
            Write-Error "health check timeout: $HealthUrl"
            exit 3
        }
    }
}

Write-Output "restart ok: app=$App service=$ServiceName"
exit 0
