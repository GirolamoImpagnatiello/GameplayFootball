param(
    [Parameter(Mandatory = $true)][string]$CaptureDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [int]$StartFrame = 1,
    [int]$FrameCount = 750,
    [long]$EventTimeMs = -1,
    [int]$EventId = -1,
    [switch]$Png,
    [string]$FfmpegPath = 'ffmpeg'
)
$ErrorActionPreference = 'Stop'
$capture = (Resolve-Path -LiteralPath $CaptureDirectory).Path
$meta = Get-Content -LiteralPath (Join-Path $capture 'metadata.json') -Raw | ConvertFrom-Json
if ($meta.capture_format -notin @('video', 'video_and_png')) { throw 'Expected direct video capture.' }
if ($EventId -ge 0) {
    if (-not $meta.event_index -or -not (Test-Path -LiteralPath $meta.event_index)) { throw 'Event index not found in capture metadata.' }
    $eventIndex = Get-Content -LiteralPath $meta.event_index -Raw | ConvertFrom-Json
    $event = @($eventIndex.events) | Where-Object { $_.event_id -eq $EventId } | Select-Object -First 1
    if (-not $event) { throw "Event id $EventId not found." }
    $EventTimeMs = [long]$event.event_actual_time_ms
    $annotationPath = Join-Path (Split-Path -Parent $meta.event_index) 'annotations.json'
    if (Test-Path -LiteralPath $annotationPath) {
        $annotations = Get-Content -LiteralPath $annotationPath -Raw | ConvertFrom-Json
        $annotation = @($annotations.event_descriptions)[$EventId]
    }
}
if ($EventTimeMs -ge 0) {
    $target = $EventTimeMs - 15000
    if ($target -lt $meta.simulation_timestamps_ms[0]) { throw 'Not enough pre-event footage.' }
    $FrameCount = 30 * [int]$meta.fps
    $StartFrame = 1
    while ($StartFrame -le $meta.frame_count -and $meta.simulation_timestamps_ms[$StartFrame - 1] -lt $target) { $StartFrame++ }
}
if ($StartFrame -lt 1 -or $FrameCount -lt 1 -or ($StartFrame + $FrameCount - 1) -gt $meta.frame_count) {
    throw 'Requested interval exceeds the recorded frames.'
}
$first = $StartFrame - 1
$last = $first + $FrameCount - 1
for ($i = $first + 1; $i -le $last; $i++) {
    if (($meta.simulation_timestamps_ms[$i] - $meta.simulation_timestamps_ms[$i-1]) * $meta.fps -ne 1000) {
        throw 'Requested clip crosses a gap in simulation time; choose an interval within one half.'
    }
}
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Choose a new output directory to avoid mixing clips.' }
$output = (New-Item -ItemType Directory -Path $OutputDirectory).FullName
$streams = @{rgb=$meta.rgb_video; depth=$meta.depth_video; seg=$meta.segmentation_video}
foreach ($channel in 'rgb','depth','seg') {
    $inputFile = Join-Path $capture $streams[$channel]
    $filter = "trim=start_frame=${first}:end_frame=$($last + 1),setpts=PTS-STARTPTS"
    $ffargs = @('-hide_banner','-loglevel','error','-nostdin','-n','-i',$inputFile,'-vf',$filter,'-an','-frames:v',$FrameCount)
    if ($Png) {
        $dir = (New-Item -ItemType Directory -Path (Join-Path $output $channel)).FullName
        $ffargs += @('-pix_fmt','rgb24','-start_number','1',(Join-Path $dir 'frame_%06d.png'))
    } else {
        $ffargs += @('-c:v','ffv1','-level','3','-threads','2','-pix_fmt','bgr0',(Join-Path $output "control_$channel.mkv"))
    }
    & $FfmpegPath @ffargs
    if ($LASTEXITCODE -ne 0) { throw "FFmpeg failed for $channel; output is incomplete." }
}
$meta.frame_count = $FrameCount
$meta.simulation_timestamps_ms = @($meta.simulation_timestamps_ms[$first..$last])
$meta.dropped_timing_buckets = 0
if ($Png) {
    $meta.capture_format = 'png'
    $meta.rgb_video = 'control_rgb.mp4'; $meta.depth_video = 'control_depth.mp4'; $meta.segmentation_video = 'control_seg.mp4'
} else {
    $meta.capture_format = 'video'
}
if (Test-Path -LiteralPath (Join-Path $capture 'prompt.json')) {
    Copy-Item -LiteralPath (Join-Path $capture 'prompt.json') -Destination (Join-Path $output 'prompt.json')
}
if (Test-Path -LiteralPath (Join-Path $capture 'cosmos_transfer_spec.json')) {
    $spec = Get-Content -LiteralPath (Join-Path $capture 'cosmos_transfer_spec.json') -Raw | ConvertFrom-Json
    $spec.num_frames = $FrameCount
    $spec.num_video_frames_per_chunk = $FrameCount
    $spec | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'cosmos_transfer_spec.json')
}
$meta | Add-Member -NotePropertyName source_capture -NotePropertyValue $capture -Force
$meta | Add-Member -NotePropertyName source_start_frame -NotePropertyValue $StartFrame -Force
$meta | Add-Member -NotePropertyName event_annotation -NotePropertyValue $annotation -Force
if ($EventTimeMs -ge 0) {
    $eventFrameIndex = 0
    while ($eventFrameIndex -lt $FrameCount -and [long]$timestamps[$first + $eventFrameIndex] -lt $EventTimeMs) { $eventFrameIndex++ }
    if ($eventFrameIndex -ge $FrameCount) { $eventFrameIndex = $FrameCount - 1 }
    $meta | Add-Member -NotePropertyName event_frame_index -NotePropertyValue ($eventFrameIndex + 1) -Force
    $meta | Add-Member -NotePropertyName event_offset_seconds -NotePropertyValue (([long]$timestamps[$first + $eventFrameIndex] - $EventTimeMs) / 1000.0) -Force
    $meta | Add-Member -NotePropertyName event_actual_time_ms -NotePropertyValue $EventTimeMs -Force
}
$meta | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'metadata.json')
if ($annotation) {
    $annotation | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'annotation.json')
}
Write-Output "Extracted $FrameCount aligned frames per channel to $output"
