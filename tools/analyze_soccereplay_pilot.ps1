param(
  [Parameter(Mandatory = $true)] [string]$DatasetRoot,
  [int]$ClipSeconds = 30,
  [string]$MatchNamePattern = '.*'
)

$ErrorActionPreference = 'Stop'
$exclude = @('start of game(half)', 'end of game(half)', 'statistics and summary', 'ball possession')
$halfClip = $ClipSeconds * 500
$clipMs = $ClipSeconds * 1000
$rows = @()
$classRows = @()

foreach ($dir in @(Get-ChildItem -LiteralPath $DatasetRoot -Directory |
    Where-Object { $_.Name -match $MatchNamePattern } | Sort-Object Name)) {
  $indexPath = Join-Path $dir.FullName 'frame_index.json'
  if (-not (Test-Path -LiteralPath $indexPath)) { continue }
  $index = Get-Content -LiteralPath $indexPath -Raw | ConvertFrom-Json
  $events = @($index.events)
  $complete = @($events | Where-Object comments_type -eq 'end of game(half)').Count -eq 2 -and
              @($events | Where-Object comments_type -eq 'statistics and summary').Count -eq 1
  if (-not $complete) { continue }

  $ends = @{}
  $starts = @{}
  foreach ($half in @(1, 2)) {
    $s = @($events | Where-Object { $_.half -eq $half -and $_.comments_type -eq 'start of game(half)' })
    $e = @($events | Where-Object { $_.half -eq $half -and $_.comments_type -eq 'end of game(half)' })
    if ($s.Count -ne 1 -or $e.Count -ne 1) { throw "Confini delle meta non validi: $($dir.Name)" }
    $starts[$half] = [double]$s[0].event_actual_time_ms
    $ends[$half] = [double]$e[0].event_actual_time_ms
  }

  $useful = @($events | Where-Object { $_.comments_type -notin $exclude })
  $valid = @($useful | Where-Object {
    $t = [double]$_.event_actual_time_ms
    $t -ge $starts[[int]$_.half] + $halfClip -and
    $t -le $ends[[int]$_.half] - $halfClip
  } | Sort-Object event_actual_time_ms)

  $selected = @()
  $lastEnd = @{}
  foreach ($half in @(1, 2)) { $lastEnd[$half] = -1.0 }
  foreach ($event in $valid) {
    $start = [double]$event.event_actual_time_ms - $halfClip
    $end = $start + $clipMs
    if ($start -ge $lastEnd[[int]$event.half]) {
      $selected += $event
      $lastEnd[[int]$event.half] = $end
    }
  }

  $uniqueMs = 0.0
  $strongPairs = 0
  $allPairs = 0
  foreach ($half in @(1, 2)) {
    $subset = @($valid | Where-Object half -eq $half)
    $unionEnd = -1.0
    for ($i = 0; $i -lt $subset.Count; $i++) {
      $start = [double]$subset[$i].event_actual_time_ms - $halfClip
      $end = $start + $clipMs
      $uniqueMs += [Math]::Max(0.0, $end - [Math]::Max($start, $unionEnd))
      $unionEnd = [Math]::Max($unionEnd, $end)
      for ($j = $i + 1; $j -lt $subset.Count; $j++) {
        $allPairs++
        if ([double]$subset[$j].event_actual_time_ms - [double]$subset[$i].event_actual_time_ms -lt $halfClip) {
          $strongPairs++
        }
      }
    }
  }

  $durationMs = $ends[2] - $starts[1]
  $rows += [pscustomobject]@{
    Match = $dir.Name
    DurationSeconds = [Math]::Round($durationMs / 1000, 2)
    RawEvents = $events.Count
    UsefulEvents = $useful.Count
    CandidateWindows = $useful.Count
    ValidWindows = $valid.Count
    NonoverlapClips = $selected.Count
    ValidClassCount = @($valid | Select-Object -ExpandProperty comments_type -Unique).Count
    UniqueCoverageSeconds = [Math]::Round($uniqueMs / 1000, 2)
    TemporalEfficiency = if ($valid.Count) { [Math]::Round($uniqueMs / ($valid.Count * $clipMs), 3) } else { 0 }
    StrongOverlapPairPct = if ($allPairs) { [Math]::Round(100 * $strongPairs / $allPairs, 1) } else { 0 }
  }
  foreach ($event in $events) {
    $classRows += [pscustomobject]@{
      Match = $dir.Name
      Label = $event.comments_type
      ValidWindow = $valid -contains $event
    }
  }
}

if (-not $rows.Count) { throw 'Nessuna partita completa trovata.' }
$rows | Format-Table -AutoSize
$totalDurationMin = ($rows | Measure-Object DurationSeconds -Sum).Sum / 60
$classRows | Group-Object Label | ForEach-Object {
  [pscustomobject]@{
    Label = $_.Name
    Events = $_.Count
    Pct = [Math]::Round(100 * $_.Count / $classRows.Count, 1)
    PerActualMinute = [Math]::Round($_.Count / $totalDurationMin, 2)
    Matches = @($_.Group | Select-Object -ExpandProperty Match -Unique).Count
    ValidWindows = @($_.Group | Where-Object ValidWindow).Count
  }
} | Sort-Object Events -Descending | Format-Table -AutoSize

$rows | Measure-Object RawEvents, UsefulEvents, CandidateWindows, ValidWindows, NonoverlapClips, DurationSeconds -Average -Sum |
  Format-Table Property,Count,Average,Sum -AutoSize
