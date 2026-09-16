param(
    [Parameter(Mandatory = $true)][string]$CaptureDirectory,
    [int]$ExpectedFrames = 250,
    [int]$ExpectedFps = 25,
    [switch]$DecodeImages
)

$ErrorActionPreference = 'Stop'
$capture = (Resolve-Path -LiteralPath $CaptureDirectory).Path
$metadata = Get-Content -LiteralPath (Join-Path $capture 'metadata.json') -Raw | ConvertFrom-Json
if (-not $metadata.lockstep) { throw 'Capture did not use lockstep.' }
if ($metadata.frame_count -ne $ExpectedFrames -or $metadata.fps -ne $ExpectedFps) {
    throw 'Unexpected frame count or frame rate.'
}
if ($metadata.dropped_timing_buckets -ne 0) { throw 'Capture skipped simulation samples.' }
$timestamps = @($metadata.simulation_timestamps_ms)
if ($timestamps.Count -ne $ExpectedFrames) { throw 'Missing simulation timestamps.' }
for ($i = 1; $i -lt $timestamps.Count; $i++) {
    $delta = [long]$timestamps[$i] - [long]$timestamps[$i - 1]
    if ($delta * $ExpectedFps -ne 1000) {
        throw "Frame $($i + 1): unexpected simulation interval of $delta ms."
    }
}

if ($DecodeImages) { Add-Type -AssemblyName System.Drawing }
$width = 0
$height = 0
foreach ($channel in 'rgb', 'depth', 'seg') {
    $directory = Join-Path $capture $channel
    $files = @(Get-ChildItem -LiteralPath $directory -Filter '*.png' -File)
    if ($files.Count -ne $ExpectedFrames) { throw "$channel has $($files.Count) images." }
    for ($i = 1; $i -le $ExpectedFrames; $i++) {
        $path = Join-Path $directory ('frame_{0:D6}.png' -f $i)
        if (-not (Test-Path -LiteralPath $path)) { throw "Missing $path" }
        if ($DecodeImages) {
            $bitmap = [System.Drawing.Bitmap]::new($path)
            try {
                if ($width -eq 0) { $width = $bitmap.Width; $height = $bitmap.Height }
                if ($bitmap.Width -ne $width -or $bitmap.Height -ne $height) {
                    throw "Inconsistent dimensions: $path"
                }
                $null = $bitmap.GetPixel($width - 1, $height - 1)
            } finally { $bitmap.Dispose() }
        }
    }
}
Write-Output "PASS: $ExpectedFrames synchronized RGB/depth/seg frames at $ExpectedFps fps; all intervals $((1000 / $ExpectedFps)) ms; no skipped samples."
