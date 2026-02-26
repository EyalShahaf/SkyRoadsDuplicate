# AGENTS.md

## Cursor Cloud specific instructions

### Overview

SkyRoads is a C++20 raylib-based 3D endless runner game. All dependencies (raylib, spdlog, nlohmann/json, backward-cpp) are fetched automatically via CMake `FetchContent`. See README.md for full architecture and commands.

### Compiler gotcha

The default `/usr/bin/c++` symlink points to Clang 18, which searches for GCC 14 C++ headers that aren't installed (only GCC 13 is present). You must pass `--gcc-install-dir=/usr/lib/gcc/x86_64-linux-gnu/13` via `CMAKE_CXX_FLAGS` and `CMAKE_C_FLAGS`. Additionally, GCC 13 fails to compile `level = {};` in `LevelLoader.cpp` (aggregate brace-init assignment regression), so Clang is the correct compiler for this environment.

The project's `CMakeLists.txt` sets `BACKWARD_HAS_DW=1` on Linux but doesn't link `-ldw`. Pass `-DCMAKE_EXE_LINKER_FLAGS="-ldw"` to cmake to fix the link step.

### Build

Build the game by running the build script from the repository root:

```bash
./scripts/build.sh
```

On Cursor Cloud VMs, the update script handles compiler and linker flags automatically. See the "Compiler gotcha" section above if you need to invoke cmake manually.

### Running the game

Run the game using the run script:

```bash
./scripts/run.sh
```

The game needs an X11 display. In headless cloud VMs, start Xvfb first:

```bash
Xvfb :99 -screen 0 1280x720x24 &
export DISPLAY=:99
./scripts/run.sh
```

### Gameplay

You control a spaceship that moves forward automatically through levels. The goal is to survive as long as possible by evading obstacles.

- **Up / Down arrow keys** — control the ship's forward velocity (speed up / slow down)
- **Left / Right arrow keys** — strafe the ship sideways to dodge obstacles
- **Space** — jump

Menu navigation uses the **arrow keys** (Up/Down/Left/Right) to move the selection and **Enter** or **Space** to confirm.

### Tests

```bash
ctest --test-dir build --output-on-failure
```

There are 5 pre-existing test failures in `sim_tests` (leaderboard-related: `submit_score_non_qualifying`, `finalize_score_entry`, `calculate_leaderboard_stats_qualifying`, `calculate_leaderboard_stats_non_qualifying`, `finish_zone_placeholder_level`). The `screenshot_test` passes. These failures are in the repository, not caused by the environment.

### Headless level validation

The `sim_runner` tool validates levels without a GUI window:

```bash
./build/sim_runner --seed 0xC0FFEE --ticks 3600 --bot cautious --quiet
```

### System dependencies required

These apt packages must be installed (beyond base Ubuntu 24.04):
- `libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev` (X11 dev headers for GLFW/raylib)
- `libgl-dev` (OpenGL dev headers)
- `libdw-dev` (elfutils/libdw for backward-cpp crash reporting)
- `libstdc++-13-dev` (C++ standard library headers)
- A `libstdc++.so` symlink may need to be created: `sudo ln -sf /usr/lib/gcc/x86_64-linux-gnu/13/libstdc++.so /usr/lib/x86_64-linux-gnu/libstdc++.so`

### Lint

This project has no separate lint tool. The compiler warnings (`-Wall -Wextra -Wpedantic`) serve as the lint check and are enabled by default in CMakeLists.txt.
