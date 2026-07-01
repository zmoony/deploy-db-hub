# encoding: utf-8
$ErrorActionPreference = 'Stop'
$mcpPath = Join-Path $env:USERPROFILE '.cursor\mcp.json'
$cfg = Get-Content $mcpPath -Raw | ConvertFrom-Json
$key = $cfg.mcpServers.stitch.headers.'X-Goog-Api-Key'
$payload = @{
    jsonrpc = '2.0'
    id      = 1
    method  = 'tools/call'
    params  = @{
        name      = 'get_screen'
        arguments = @{
            projectId = '12185237422534899983'
            screenId  = '608e9da4d3054134a381b10fbada2254'
        }
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

$response | ConvertTo-Json -Depth 30 | Out-File -Encoding utf8 $args[0]
