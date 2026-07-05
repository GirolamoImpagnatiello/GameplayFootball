param(
  [Parameter(Mandatory = $true)]
  [string]$CaptureDirectory,

  [int]$Fps = 30,

  [string]$FfmpegPath = "ffmpeg",

  [int]$StartFrame = 1,

  [int]$FrameCount = 0
)

$ErrorActionPreference = "Stop"

if ($FfmpegPath -eq "ffmpeg") {
  $ffmpegCommand = Get-Command ffmpeg -ErrorAction SilentlyContinue
  if (-not $ffmpegCommand) {
    throw "ffmpeg non trovato. Installa ffmpeg e aggiungilo al PATH, oppure rilancia lo script con -FfmpegPath C:\percorso\ffmpeg.exe"
  }
  $FfmpegPath = $ffmpegCommand.Source
} elseif (-not (Test-Path -LiteralPath $FfmpegPath)) {
  throw "ffmpeg non trovato nel percorso specificato: $FfmpegPath"
}

function Invoke-FfmpegSequence {
  param(
    [string]$InputDirectory,
    [string]$OutputPath
  )

  $pattern = Join-Path $InputDirectory "frame_%06d.png"
  $args = @("-y", "-framerate", $Fps, "-start_number", $StartFrame, "-i", $pattern)
  if ($FrameCount -gt 0) {
    $args += @("-frames:v", $FrameCount)
  }
  $args += @("-c:v", "libx264", "-pix_fmt", "yuv420p", "-movflags", "+faststart", $OutputPath)
  & $FfmpegPath @args
  if ($LASTEXITCODE -ne 0) {
    throw "ffmpeg failed for $InputDirectory"
  }
}

$capture = Resolve-Path -LiteralPath $CaptureDirectory
$metadataPath = Join-Path $capture "metadata.json"
if (Test-Path -LiteralPath $metadataPath) {
  $metadata = Get-Content -Raw -LiteralPath $metadataPath | ConvertFrom-Json
  if ($metadata.fps) {
    $Fps = [int]$metadata.fps
  }
  if ($FrameCount -le 0 -and $metadata.frame_count) {
    $FrameCount = [int]$metadata.frame_count
  }
}

Invoke-FfmpegSequence -InputDirectory (Join-Path $capture "rgb") -OutputPath (Join-Path $capture "control_rgb.mp4")
Invoke-FfmpegSequence -InputDirectory (Join-Path $capture "depth") -OutputPath (Join-Path $capture "control_depth.mp4")
Invoke-FfmpegSequence -InputDirectory (Join-Path $capture "seg") -OutputPath (Join-Path $capture "control_seg.mp4")

Write-Host "Cosmos videos written under $capture"
