cmake_minimum_required(VERSION 3.21)

project(SkyRoads
    VERSION 0.1.0
    DESCRIPTION "SkyRoads-like runner experiment with raylib"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
FetchContent_MakeAvailable(raylib)

add_executable(skyroads
    src/main.cpp
    core/Rng.cpp
    core/PerfTracker.cpp
    core/Assets.cpp
    game/Game.cpp
    sim/Sim.cpp
    sim/Level.cpp
    render/Palette.cpp
    render/Render.cpp
)

target_compile_features(skyroads PRIVATE cxx_std_20)
target_link_libraries(skyroads PRIVATE raylib)
target_include_directories(skyroads PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(skyroads PRIVATE /W4 /permissive-)
else()
    target_compile_options(skyroads PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(sim_tests
    tests/SimTests.cpp
    game/Game.cpp
    sim/Sim.cpp
    sim/Level.cpp
    core/Rng.cpp
    core/Assets.cpp
    render/Palette.cpp
    render/Render.cpp
)
target_compile_features(sim_tests PRIVATE cxx_std_20)
target_link_libraries(sim_tests PRIVATE raylib)
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
    sim/Sim.cpp
    sim/Level.cpp
    sim/Bot.cpp
    core/Rng.cpp
    core/Assets.cpp
    render/Palette.cpp
    render/Render.cpp
)
target_compile_features(sim_runner PRIVATE cxx_std_20)
target_link_libraries(sim_runner PRIVATE raylib)
target_include_directories(sim_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(sim_runner PRIVATE /W4 /permissive-)
else()
    target_compile_options(sim_runner PRIVATE -Wall -Wextra -Wpedantic)
endif()
