cmake_minimum_required(VERSION 3.21)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(SkyRoads
    VERSION 0.1.0
    DESCRIPTION "SkyRoads-like runner experiment with raylib"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOS-specific: Configure SDK and compiler settings
if(APPLE)
    # Try to use xcrun to find the SDK path
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(MACOS_SDK_PATH)
        set(CMAKE_OSX_SYSROOT "${MACOS_SDK_PATH}" CACHE PATH "macOS SDK path")
        message(STATUS "Using macOS SDK: ${MACOS_SDK_PATH}")
    endif()
    
    # Set minimum deployment target
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum macOS deployment target")
    endif()
    
    # Set SDKROOT environment variable for subprocesses (helps with raylib/GLFW builds)
    if(MACOS_SDK_PATH)
        set(ENV{SDKROOT} "${MACOS_SDK_PATH}")
    endif()
endif()

include(FetchContent)

# Keep raylib local to this build tree and avoid noisy updates after first fetch.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    backward
    GIT_REPOSITORY https://github.com/bombela/backward-cpp.git
    GIT_TAG v1.6
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)

FetchContent_MakeAvailable(raylib spdlog backward json)

add_executable(skyroads
    src/main.cpp
    core/Config.cpp
    core/Rng.cpp
    core/PerfTracker.cpp
    core/Assets.cpp
    core/Log.cpp
    core/CrashHandler.cpp
    game/Game.cpp
    game/Leaderboard.cpp
    sim/Sim.cpp
    sim/Level.cpp
    sim/LevelGeometry.cpp
    sim/LevelLoader.cpp
    sim/LevelVariantAssigner.cpp
    sim/BuiltinLevels.cpp
    sim/EndlessLevelGenerator.cpp
    render/Palette.cpp
    render/SpaceObjects.cpp
    render/SceneDressing.cpp
    render/HudWidgets.cpp
    render/GateRenderer.cpp
    render/Render.cpp
)

target_compile_features(skyroads PRIVATE cxx_std_20)
target_link_libraries(skyroads PRIVATE raylib spdlog::spdlog backward nlohmann_json::nlohmann_json)
target_include_directories(skyroads PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# backward-cpp needs some definitions to work correctly
if(APPLE)
    target_compile_definitions(skyroads PRIVATE BACKWARD_HAS_UNWIND=1 BACKWARD_HAS_BACKTRACE_SYMBOL=1)
elseif(LINUX)
    target_compile_definitions(skyroads PRIVATE BACKWARD_HAS_DW=1)
endif()

if(MSVC)
    target_compile_options(skyroads PRIVATE /W4 /permissive-)
else()
    target_compile_options(skyroads PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(sim_tests
    tests/SimTests.cpp
    game/Game.cpp
    game/Leaderboard.cpp
    sim/Sim.cpp
    sim/Level.cpp
    sim/LevelGeometry.cpp
    sim/LevelLoader.cpp
    sim/LevelVariantAssigner.cpp
    sim/BuiltinLevels.cpp
    sim/EndlessLevelGenerator.cpp
    core/Config.cpp
    core/Rng.cpp
    core/Assets.cpp
    core/Log.cpp
    render/Palette.cpp
    render/SpaceObjects.cpp
    render/SceneDressing.cpp
    render/HudWidgets.cpp
    render/GateRenderer.cpp
    render/Render.cpp
)
target_compile_features(sim_tests PRIVATE cxx_std_20)
target_link_libraries(sim_tests PRIVATE raylib spdlog::spdlog nlohmann_json::nlohmann_json)
target_include_directories(sim_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(sim_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(sim_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME sim_tests COMMAND sim_tests)

# Screenshot functionality test
add_executable(screenshot_test
    tests/ScreenshotTest.cpp
)
target_compile_features(screenshot_test PRIVATE cxx_std_20)
target_link_libraries(screenshot_test PRIVATE raylib)
target_include_directories(screenshot_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(screenshot_test PRIVATE /W4 /permissive-)
else()
    target_compile_options(screenshot_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME screenshot_test COMMAND screenshot_test)

# Headless sim runner for level validation (no window needed).
add_executable(sim_runner
    tools/sim_runner.cpp
    game/Game.cpp
    game/Leaderboard.cpp
    sim/Sim.cpp
    sim/Level.cpp
    sim/LevelGeometry.cpp
    sim/LevelLoader.cpp
    sim/LevelVariantAssigner.cpp
    sim/BuiltinLevels.cpp
    sim/EndlessLevelGenerator.cpp
    sim/Bot.cpp
    core/Config.cpp
    core/Rng.cpp
    core/Assets.cpp
    core/Log.cpp
    render/Palette.cpp
    render/SpaceObjects.cpp
    render/SceneDressing.cpp
    render/HudWidgets.cpp
    render/GateRenderer.cpp
    render/Render.cpp
)
target_compile_features(sim_runner PRIVATE cxx_std_20)
target_link_libraries(sim_runner PRIVATE raylib spdlog::spdlog nlohmann_json::nlohmann_json)
target_include_directories(sim_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(sim_runner PRIVATE /W4 /permissive-)
else()
    target_compile_options(sim_runner PRIVATE -Wall -Wextra -Wpedantic)
endif()
