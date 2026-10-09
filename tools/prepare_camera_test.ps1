param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectDirectory
)

$project = (Resolve-Path -LiteralPath $ProjectDirectory).Path
$sourceConfig = Join-Path $project 'data\football.config'
$buildDirectory = Join-Path $project 'out\build-vs2026-x86'
$testConfig = Join-Path $buildDirectory 'camera-test.config'

$text = Get-Content -Raw -LiteralPath $sourceConfig
$values = [ordered]@{
    debug_camera_scenario_enabled = 'true'
    debug_save_to_corner_sequence = 'false'
    camera_mode = 'procedural'
    single_camera = 'false'
    camera_changes = 'true'
    camera_debug_overlay = 'true'
    random_seed = '2026'
    match_duration_minutes = '2'
    automatic_match_count = '1'
    automatic_quit_when_done = 'true'
}

foreach ($entry in $values.GetEnumerator()) {
    $pattern = '(?m)^"' + [regex]::Escape($entry.Key) + '"\s+"[^"]*"'
    $replacement = '"' + $entry.Key + '" "' + $entry.Value + '"'
    if (-not [regex]::IsMatch($text, $pattern)) {
        throw "Impostazione mancante nella configurazione: $($entry.Key)"
    }
    $text = [regex]::Replace($text, $pattern, $replacement)
}

Set-Content -LiteralPath $testConfig -Value $text -Encoding UTF8
