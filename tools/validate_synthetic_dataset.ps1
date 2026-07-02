param(
  [Parameter(Mandatory = $true)]
  [string]$MatchDirectory
)

$ErrorActionPreference = "Stop"

$canonicalLabels = @(
  "corner",
  "goal",
  "injury",
  "own goal",
  "penalty",
  "penalty missed",
  "red card",
  "second yellow card",
  "substitution",
  "start of game(half)",
  "end of game(half)",
  "yellow card",
  "throw in",
  "free kick",
  "saved by goal-keeper",
  "shot off target",
  "clearance",
  "lead to corner",
  "off-side",
  "var",
  "foul (no card)",
  "statistics and summary",
  "ball possession",
  "ball out of play"
)

function Assert-Condition {
  param(
    [bool]$Condition,
    [string]$Message
  )
  if (-not $Condition) {
    throw $Message
  }
}

function Assert-PngHasOpaquePixels {
  param(
    [string]$Path,
    [string]$Label
  )

  Add-Type -AssemblyName System.Drawing
  $bitmap = [System.Drawing.Bitmap]::FromFile($Path)
  try {
    $hasOpaquePixel = $false
    $stepX = [Math]::Max(1, [int]($bitmap.Width / 8))
    $stepY = [Math]::Max(1, [int]($bitmap.Height / 8))
    for ($y = 0; $y -lt $bitmap.Height -and -not $hasOpaquePixel; $y += $stepY) {
      for ($x = 0; $x -lt $bitmap.Width -and -not $hasOpaquePixel; $x += $stepX) {
        if ($bitmap.GetPixel($x, $y).A -gt 0) {
          $hasOpaquePixel = $true
        }
      }
    }
    Assert-Condition $hasOpaquePixel "$Label ha alpha completamente trasparente"
  } finally {
    $bitmap.Dispose()
  }
}

$annotationsPath = Join-Path $MatchDirectory "annotations.json"
$frameIndexPath = Join-Path $MatchDirectory "frame_index.json"
$framesPath = Join-Path $MatchDirectory "frames"
$clipsPath = Join-Path $MatchDirectory "clips"

Assert-Condition (Test-Path -LiteralPath $annotationsPath) "annotations.json mancante"
Assert-Condition (Test-Path -LiteralPath $frameIndexPath) "frame_index.json mancante"
Assert-Condition (Test-Path -LiteralPath $framesPath) "cartella frames mancante"
Assert-Condition (Test-Path -LiteralPath $clipsPath) "cartella clips mancante"

$annotations = Get-Content -Raw -LiteralPath $annotationsPath | ConvertFrom-Json
$frameIndex = Get-Content -Raw -LiteralPath $frameIndexPath | ConvertFrom-Json

Assert-Condition ($null -ne $annotations.match_information) "match_information mancante"
Assert-Condition ($null -ne $annotations.referee_information) "referee_information mancante"
Assert-Condition ($null -ne $annotations.player_information) "player_information mancante"
Assert-Condition ($null -ne $annotations.event_descriptions) "event_descriptions mancante"

foreach ($event in $annotations.event_descriptions) {
  Assert-Condition ($event.half -eq 1 -or $event.half -eq 2) "half non valido in annotations.json"
  Assert-Condition ($event.time_stamp -match '^\d{2}:\d{2}$') "time_stamp non valido: $($event.time_stamp)"
  Assert-Condition ($canonicalLabels -contains $event.comments_type) "label non canonica: $($event.comments_type)"
}

Assert-Condition ($frameIndex.fps -eq 1) "frame_index fps deve essere 1"
Assert-Condition ($frameIndex.window_duration_seconds -eq 30) "window_duration_seconds deve essere 30"
Assert-Condition ($frameIndex.window_start_offset_seconds -eq -15) "window_start_offset_seconds deve essere -15"
Assert-Condition ($frameIndex.window_end_offset_seconds -eq 14) "window_end_offset_seconds deve essere 14"
Assert-Condition ($null -ne $frameIndex.frames) "frames mancante in frame_index.json"
Assert-Condition ($null -ne $frameIndex.events) "events mancante in frame_index.json"

$frames = @($frameIndex.frames)
$events = @($frameIndex.events)
Assert-Condition ($frames.Count -gt 0) "nessun frame registrato"
Assert-Condition ($events.Count -eq @($annotations.event_descriptions).Count) "numero eventi non allineato tra annotations.json e frame_index.json"

for ($i = 0; $i -lt $frames.Count; $i++) {
  $frame = $frames[$i]
  Assert-Condition ($frame.frame_number -eq ($i + 1)) "frame_number non sequenziale al frame $($i + 1)"
  Assert-Condition ($frame.time_stamp -match '^\d{2}:\d{2}$') "time_stamp frame non valido: $($frame.time_stamp)"
  Assert-Condition ($frame.half -eq 1 -or $frame.half -eq 2) "half frame non valido"
  $frameFile = Join-Path $MatchDirectory $frame.file
  Assert-Condition (Test-Path -LiteralPath $frameFile) "file frame mancante: $($frame.file)"
  if ($i -eq 0 -or $i -eq [int]($frames.Count / 2) -or $i -eq ($frames.Count - 1)) {
    Assert-PngHasOpaquePixels -Path $frameFile -Label "frame $($frame.file)"
  }
}

$framesByHalf = $frames | Group-Object half
foreach ($group in $framesByHalf) {
  $ordered = @($group.Group | Sort-Object frame_number)
  for ($i = 1; $i -lt $ordered.Count; $i++) {
    $delta = [int64]$ordered[$i].actual_time_ms - [int64]$ordered[$i - 1].actual_time_ms
    Assert-Condition ($delta -ge 500 -and $delta -le 1500) "frame dump non a 1 fps video nel half $($group.Name): delta ${delta}ms"
  }
}

foreach ($event in $events) {
  Assert-Condition ($event.fps -eq 1) "evento $($event.event_id): fps deve essere 1"
  Assert-Condition ($event.window_duration_seconds -eq 30) "evento $($event.event_id): finestra non da 30 secondi"
  Assert-Condition ($event.window_start_offset_seconds -eq -15) "evento $($event.event_id): offset iniziale deve essere -15"
  Assert-Condition ($event.window_end_offset_seconds -eq 14) "evento $($event.event_id): offset finale deve essere 14"
  Assert-Condition ($event.event_inside_window -eq $true) "evento $($event.event_id): evento fuori dalla finestra"
  Assert-Condition ($event.clip_frame_count -eq 30) "evento $($event.event_id): clip_frame_count deve essere 30"
  Assert-Condition ($event.frame_count -eq 30) "evento $($event.event_id): frame_count deve essere 30"
  Assert-Condition ($null -ne $event.clip_frames) "evento $($event.event_id): clip_frames mancante"

  $clipFrames = @($event.clip_frames)
  Assert-Condition ($clipFrames.Count -eq 30) "evento $($event.event_id): la clip deve contenere 30 frame"
  Assert-Condition ([int64]$event.event_actual_time_ms -ge [int64]$event.window_start_actual_time_ms) "evento $($event.event_id): timestamp prima della finestra"
  Assert-Condition ([int64]$event.event_actual_time_ms -lt [int64]$event.window_end_actual_time_ms) "evento $($event.event_id): timestamp dopo la finestra"

  for ($i = 0; $i -lt $clipFrames.Count; $i++) {
    $clipFrame = $clipFrames[$i]
    $expectedNumber = $i + 1
    $expectedOffset = $i - 15
    $expectedTarget = [int64]$event.event_actual_time_ms + ([int64]$expectedOffset * 1000)

    Assert-Condition ($clipFrame.clip_frame_number -eq $expectedNumber) "evento $($event.event_id): clip_frame_number non sequenziale"
    Assert-Condition ($clipFrame.offset_seconds -eq $expectedOffset) "evento $($event.event_id): offset non valido al clip frame $expectedNumber"
    Assert-Condition ([int64]$clipFrame.target_actual_time_ms -eq $expectedTarget) "evento $($event.event_id): target_actual_time_ms non allineato al clip frame $expectedNumber"

    $sourceFrame = @($frames | Where-Object { $_.frame_number -eq $clipFrame.source_frame_number })
    Assert-Condition ($sourceFrame.Count -eq 1) "evento $($event.event_id): source_frame_number non presente nei frame globali"
    Assert-Condition ($sourceFrame[0].half -eq $event.half) "evento $($event.event_id): source frame in half errato"

    $sourceFile = Join-Path $MatchDirectory $clipFrame.source_file
    $clipFile = Join-Path $MatchDirectory $clipFrame.clip_file
    Assert-Condition (Test-Path -LiteralPath $sourceFile) "evento $($event.event_id): source_file mancante: $($clipFrame.source_file)"
    Assert-Condition (Test-Path -LiteralPath $clipFile) "evento $($event.event_id): clip_file mancante: $($clipFrame.clip_file)"
  }
}

Write-Host "Dataset sintetico valido: $MatchDirectory"
Write-Host "Eventi: $(@($annotations.event_descriptions).Count)"
Write-Host "Frame: $($frames.Count)"
