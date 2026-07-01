# encoding: utf-8
$ErrorActionPreference = 'Stop'
$mcpPath = Join-Path $env:USERPROFILE '.cursor\mcp.json'
$cfg = Get-Content $mcpPath -Raw | ConvertFrom-Json
$key = $cfg.mcpServers.stitch.headers.'X-Goog-Api-Key'
if ([string]::IsNullOrWhiteSpace($key)) {
    Write-Error 'Stitch API key not found in ~/.cursor/mcp.json'
    exit 1
}

$payload = @{
    jsonrpc = '2.0'
    id      = 1
    method  = 'tools/call'
    params  = @{
        name      = 'list_projects'
        arguments = @{}
    }
} | ConvertTo-Json -Depth 5 -Compress

$response = Invoke-RestMethod `
    -Uri 'https://stitch.googleapis.com/mcp' `
    -Method Post `
    -Headers @{
        'Content-Type'  = 'application/json'
        'X-Goog-Api-Key' = $key
    } `
    -Body $payload

$response | ConvertTo-Json -Depth 20
