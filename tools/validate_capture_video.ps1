param(
    [Parameter(Mandatory=$true)][string]$CaptureDirectory,
    [int]$ExpectedFrames=750,
    [int]$ExpectedFps=25,
    [string]$FfprobePath='ffprobe',
    [string]$FfmpegPath='ffmpeg'
)
$ErrorActionPreference='Stop'
$capture=(Resolve-Path -LiteralPath $CaptureDirectory).Path
$meta=Get-Content -Raw -LiteralPath (Join-Path $capture 'metadata.json') | ConvertFrom-Json
if (-not $meta.lockstep -or -not $meta.team_aware) { throw 'Lockstep and team-aware capture are required.' }
if ($meta.frame_count -ne $ExpectedFrames -or $meta.fps -ne $ExpectedFps) { throw 'Unexpected capture size/rate.' }
if ($meta.event_frame_index -and ($meta.event_frame_index -lt 1 -or $meta.event_frame_index -gt $ExpectedFrames)) { throw 'Event is outside extracted clip.' }
if ($meta.dropped_timing_buckets -ne 0) { throw 'Skipped simulation samples.' }
if (@($meta.simulation_timestamps_ms).Count -ne $ExpectedFrames) { throw 'Missing timestamps.' }
for($i=1;$i -lt $ExpectedFrames;$i++) {
    if (($meta.simulation_timestamps_ms[$i]-$meta.simulation_timestamps_ms[$i-1])*$ExpectedFps -ne 1000) { throw "Invalid simulation interval at frame $($i+1)." }
}
foreach($file in @($meta.rgb_video,$meta.depth_video,$meta.segmentation_video)) {
    $path=Join-Path $capture $file
    $json=& $FfprobePath -v error -select_streams v:0 -count_frames -show_entries stream=codec_name,width,height,r_frame_rate,nb_read_frames -of json $path
    if($LASTEXITCODE -ne 0) { throw "Cannot probe $file" }
    $stream=($json | ConvertFrom-Json).streams[0]
    if($stream.codec_name -ne 'ffv1' -or $stream.nb_read_frames -ne $ExpectedFrames -or $stream.r_frame_rate -ne "$ExpectedFps/1" -or $stream.width -ne 1280 -or $stream.height -ne 720) { throw "Invalid stream: $file" }
    & $FfmpegPath -v error -xerror -i $path -f null -
    if($LASTEXITCODE -ne 0) { throw "Corrupt video: $file" }
}
Write-Output "PASS: three lossless 720p streams, $ExpectedFrames frames each, $ExpectedFps fps, exact simulation intervals and no dropped samples."
