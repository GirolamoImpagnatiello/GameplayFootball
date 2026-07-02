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

$annotationsPath = Join-Path $MatchDirectory "annotations.json"
$frameIndexPath = Join-Path $MatchDirectory "frame_index.json"
$framesPath = Join-Path $MatchDirectory "frames"

Assert-Condition (Test-Path -LiteralPath $annotationsPath) "annotations.json mancante"
Assert-Condition (Test-Path -LiteralPath $frameIndexPath) "frame_index.json mancante"
Assert-Condition (Test-Path -LiteralPath $framesPath) "cartella frames mancante"

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
}

$framesByHalf = $frames | Group-Object half
foreach ($group in $framesByHalf) {
  $ordered = @($group.Group | Sort-Object frame_number)
  for ($i = 1; $i -lt $ordered.Count; $i++) {
    $delta = [int64]$ordered[$i].match_time_ms - [int64]$ordered[$i - 1].match_time_ms
    Assert-Condition ($delta -ge 500 -and $delta -le 1500) "frame dump non a 1 fps nel half $($group.Name): delta ${delta}ms"
  }
}

foreach ($event in $events) {
  Assert-Condition ($event.fps -eq 1) "evento $($event.event_id): fps deve essere 1"
  Assert-Condition ($event.window_duration_seconds -eq 30) "evento $($event.event_id): finestra non da 30 secondi"
  Assert-Condition ($event.event_inside_window -eq $true) "evento $($event.event_id): evento fuori dalla finestra"
  Assert-Condition ($event.frame_start -le $event.frame_center -and $event.frame_center -le $event.frame_end) "evento $($event.event_id): frame_center fuori range"
  Assert-Condition ($event.frame_count -eq ($event.frame_end - $event.frame_start + 1)) "evento $($event.event_id): frame_count incoerente"

  $halfFrameCount = @($frames | Where-Object { $_.half -eq $event.half }).Count
  $expectedMax = [Math]::Min(30, $halfFrameCount)
  Assert-Condition ($event.frame_count -eq $expectedMax) "evento $($event.event_id): frame_count deve essere $expectedMax"
}

Write-Host "Dataset sintetico valido: $MatchDirectory"
Write-Host "Eventi: $(@($annotations.event_descriptions).Count)"
Write-Host "Frame: $($frames.Count)"
