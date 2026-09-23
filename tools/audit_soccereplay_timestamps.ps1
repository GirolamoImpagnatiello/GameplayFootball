param(
  [Parameter(Mandatory = $true)]
  [string]$Path
)

$ErrorActionPreference = 'Stop'
$inputPath = (Resolve-Path -LiteralPath $Path).Path
$matchDirs = if (Test-Path -LiteralPath (Join-Path $inputPath 'frame_index.json')) {
  @(Get-Item -LiteralPath $inputPath)
} else {
  @(Get-ChildItem -LiteralPath $inputPath -Directory | Where-Object {
    Test-Path -LiteralPath (Join-Path $_.FullName 'frame_index.json')
  })
}

if ($matchDirs.Count -eq 0) { throw 'Nessun frame_index.json trovato.' }

$hasErrors = $false
foreach ($dir in $matchDirs) {
  $data = Get-Content -LiteralPath (Join-Path $dir.FullName 'frame_index.json') -Raw | ConvertFrom-Json
  $events = @($data.events)
  $issues = @()
  $ids = @($events | Select-Object -ExpandProperty event_id)
  if (@($ids | Select-Object -Unique).Count -ne $ids.Count) {
    $issues += 'event_id duplicati'
  }
  foreach ($event in $events) {
    if ($null -eq $event.event_actual_time_ms -or $event.event_actual_time_ms -lt 0) {
      $issues += "timestamp reale non valido per event_id=$($event.event_id)"
    }
  }
  foreach ($group in @($events | Group-Object half, event_actual_time_ms)) {
    $labels = @($group.Group | Select-Object -ExpandProperty comments_type)
    $foulLabels = @('foul (no card)', 'yellow card', 'second yellow card', 'red card')
    if (@($labels | Where-Object { $_ -in $foulLabels }).Count -gt 1) {
      $issues += "piu classi di fallo allo stesso istante: $($group.Name)"
    }
    if (@($labels | Where-Object { $_ -in @('free kick', 'penalty') }).Count -gt 0 -and
        @($labels | Where-Object { $_ -in ($foulLabels + @('off-side')) }).Count -gt 0) {
      $issues += "causa e calcio da fermo allo stesso istante: $($group.Name)"
    }
  }

  [pscustomobject]@{
    Match = $dir.Name
    Events = $events.Count
    Issues = $issues.Count
    Details = $issues -join '; '
  }
  if ($issues.Count -gt 0) { $hasErrors = $true }
}

if ($hasErrors) { exit 1 }
