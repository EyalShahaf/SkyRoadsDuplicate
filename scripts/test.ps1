$ErrorActionPreference = "Stop"

$BuildDir = if ($args.Count -gt 0) { $args[0] } else { "build" }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { "Release" }

cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=$BuildType -DFETCHCONTENT_QUIET=OFF
cmake --build $BuildDir --config $BuildType
ctest --test-dir $BuildDir --output-on-failure
