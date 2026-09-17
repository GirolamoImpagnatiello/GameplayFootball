param(
    [Parameter(Mandatory = $true)][string]$CaptureDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [switch]$Png,
    [switch]$IncludeBoundaryEvents,
    [string]$FfmpegPath = 'ffmpeg'
)

$ErrorActionPreference = 'Stop'
$capture = (Resolve-Path -LiteralPath $CaptureDirectory).Path
$meta = Get-Content -LiteralPath (Join-Path $capture 'metadata.json') -Raw | ConvertFrom-Json
if (-not $meta.event_index -or -not (Test-Path -LiteralPath $meta.event_index)) { throw 'Event index not found in metadata.json.' }
$index = Get-Content -LiteralPath $meta.event_index -Raw | ConvertFrom-Json
if (Test-Path -LiteralPath $OutputDirectory) { throw 'OutputDirectory already exists; choose a new directory.' }
$null = New-Item -ItemType Directory -Path $OutputDirectory

$timestamps = @($meta.simulation_timestamps_ms)
$firstTime = [long]$timestamps[0]
$lastTime = [long]$timestamps[-1]
$ok = 0; $skipped = 0
foreach ($event in @($index.events)) {
    $eventTime = [long]$event.event_actual_time_ms
    if (-not $IncludeBoundaryEvents -and ($eventTime - 15000 -lt $firstTime -or $eventTime + 14000 -gt $lastTime)) {
        Write-Warning "Skipping event $($event.event_id) ($($event.comments_type)): insufficient 30-second context."
        $skipped++; continue
    }
    $eventDir = Join-Path $OutputDirectory ('event_{0:D6}' -f [int]$event.event_id)
    try {
        & (Join-Path $PSScriptRoot 'extract_capture_video.ps1') -CaptureDirectory $capture -OutputDirectory $eventDir -EventId ([int]$event.event_id) -Png:$Png -FfmpegPath $FfmpegPath
        if ($LASTEXITCODE -ne 0) { throw "extractor exit code $LASTEXITCODE" }
        $ok++
    } catch {
        Write-Warning "Event $($event.event_id) failed: $($_.Exception.Message)"
        if (Test-Path -LiteralPath $eventDir) { Remove-Item -LiteralPath $eventDir -Recurse -Force }
        $skipped++
    }
}
Write-Output "Extracted $ok event clips; skipped $skipped."
