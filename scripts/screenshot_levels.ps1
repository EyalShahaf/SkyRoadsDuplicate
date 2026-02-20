# Screenshot automation script for SkyRoads (PowerShell)
# Generates screenshots for all levels with different palettes

param(
    [string]$BuildDir = "build",
    [string]$OutputDir = "docs/screenshots-raw",
    [string]$Seed = "0xC0FFEE",
    [int]$Interval = 2400  # Screenshot every 20 seconds at 120Hz
)

$ErrorActionPreference = "Stop"

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Cyan -NoNewline
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Info {
    param([string]$Message)
    Write-Host "ℹ️  $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "✅ $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "⚠️  $Message" -ForegroundColor Yellow
}

# Check if sim_runner exists
$simRunnerPath = Join-Path $BuildDir "sim_runner.exe"
if (-not (Test-Path $simRunnerPath)) {
    Write-Warn "sim_runner not found. Building..."
    & .\scripts\build.ps1 $BuildDir
}

Write-Header "SkyRoads Screenshot Automation"

Write-Info "Output directory: $OutputDir"
Write-Info "Seed: $Seed"
Write-Info "Screenshot interval: $Interval ticks (every ~20 seconds)"

# Create output directory
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$totalLevels = 30
$totalPalettes = 3
$total = $totalLevels * $totalPalettes
$current = 0

$startTime = Get-Date

for ($level = 1; $level -le $totalLevels; $level++) {
    for ($palette = 0; $palette -lt $totalPalettes; $palette++) {
        $current++
        
        $levelDir = Join-Path $OutputDir "level_$level\palette_$palette"
        New-Item -ItemType Directory -Force -Path $levelDir | Out-Null
        
        Write-Info "[$current/$total] Level $level, Palette $palette..."
        
        # Run sim_runner with screenshots
        & $simRunnerPath `
            --seed $Seed `
            --level $level `
            --palette $palette `
            --screenshots `
            --screenshot-output $levelDir `
            --screenshot-interval $Interval `
            --ticks 12000 `
            --quiet
        
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "sim_runner exited with code $LASTEXITCODE (bot may have died)"
        } else {
            Write-Success "Completed level $level, palette $palette"
        }
    }
}

$endTime = Get-Date
$elapsed = $endTime - $startTime

Write-Header "Screenshot Generation Complete"
Write-Success "Generated screenshots for $total level/palette combinations"
Write-Info "Total time: $($elapsed.Minutes)m $($elapsed.Seconds)s"
Write-Info "Screenshots saved to: $OutputDir"
