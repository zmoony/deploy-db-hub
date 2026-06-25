$prompt = "kawaii cartoon office worker waiting for reimbursement, cute big eyes, empty wallet, calendar with seasons passed, snail carrying money sign, funny humorous, pastel colors, thick outlines, Japanese cute style, simple clean background, speech bubble"

$encoded = [System.Uri]::EscapeDataString($prompt)
$imageSize = "portrait_16_9"
$outputPath = "d:\project\code\vibe-coding\deploy-hub\infographic\quarterly-reimbursement\infographic.png"

$url = "https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=$encoded&image_size=$imageSize"

Write-Host "Generating image (attempt 2)..."

Invoke-WebRequest -Uri $url -OutFile $outputPath -TimeoutSec 180

Write-Host "Image saved to: $outputPath"
if (Test-Path $outputPath) {
    $size = (Get-Item $outputPath).Length
    Write-Host "File size: $([math]::Round($size/1024, 2)) KB"
    $bytes = [System.IO.File]::ReadAllBytes($outputPath)
    $header = [System.Text.Encoding]::ASCII.GetString($bytes[0..7])
    Write-Host "File header: $header"
    if ($header -notmatch "PNG") {
        $textContent = [System.Text.Encoding]::UTF8.GetString($bytes)
        if ($textContent -match "html|HTML|生成|刷新") {
            Write-Host "WARNING: File appears to be HTML/placeholder, not a real image!"
        }
    }
}
