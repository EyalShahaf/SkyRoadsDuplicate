$ErrorActionPreference = "Stop"

$BuildDir = if ($args.Count -gt 0) { $args[0] } else { "build" }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Release" }

$candidateA = Join-Path $BuildDir "skyroads.exe"
$candidateB = Join-Path (Join-Path $BuildDir $BuildType) "skyroads.exe"

if (Test-Path $candidateA) {
    & $candidateA
} elseif (Test-Path $candidateB) {
    & $candidateB
} else {
    Write-Error "Could not find built executable. Run ./scripts/build.ps1 first."
}
