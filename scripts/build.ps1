$ErrorActionPreference = "Stop"

$BuildDir = if ($args.Count -gt 0) { $args[0] } else { "build" }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Release" }

Write-Host "Configuring ($BuildType)..."
Write-Host "Note: first run downloads raylib and can take a little while."
cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=$BuildType -DFETCHCONTENT_QUIET=OFF
Write-Host "Building..."
cmake --build $BuildDir --config $BuildType

Write-Host "Build complete: $BuildDir"
