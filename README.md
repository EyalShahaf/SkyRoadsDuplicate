# SkyRoads — Endless Runner 🚀

![C++](https://img.shields.io/badge/C++-20-blue?style=for-the-badge&logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.21+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
![raylib](https://img.shields.io/badge/raylib-5.5-FF6B6B?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)

A retro-inspired SkyRoads-style endless runner built with **C++20** and **raylib**.
Fly a speeder ship across a neon platform, dash for style points, and climb the leaderboard.

---

<div align="center">

# 🤖 Built with AI Assistance

**This entire project was developed collaboratively with AI coding assistants, including Cursor AI and other notable AI agents.**
**The development process showcased the power of human-AI collaboration in modern software engineering.**

</div>

---

## 📑 Table of Contents

- [Highlights](#-highlights)
- [Screenshots](#-screenshots)
- [Features](#-features)
- [Tech Stack](#-tech-stack)
- [Controls](#-controls)
- [Quick Start](#-quick-start)
- [Architecture](#-architecture)
- [Requirements](#-requirements)
- [Credits](#-credits)

## 🌟 Highlights

- ⚡ **High Performance** - Fixed timestep at 120Hz with zero-allocation game loop
- 🎨 **Stunning Visuals** - Neon aesthetics with particle effects and dynamic lighting
- 🎯 **Deterministic** - Seeded runs for perfect reproducibility and testing
- 🧪 **Well Tested** - 14 automated simulation tests ensuring game mechanics work correctly
- 🎮 **Smooth Controls** - Coyote time and input buffering for responsive, forgiving gameplay
- 🚀 **Cross-Platform** - Runs seamlessly on Windows, macOS, and Linux
- 🎭 **Multiple Palettes** - Three distinct color schemes to match your mood

## 📸 Screenshots

<div align="center">
  <img src="docs/screenshots/screenshot_20260219_201540.png" width="45%" alt="Gameplay Screenshot 1"/>
  <img src="docs/screenshots/screenshot_20260219_201545.png" width="45%" alt="Gameplay Screenshot 2"/>
  <br/>
  <img src="docs/screenshots/screenshot_20260219_213923.png" width="45%" alt="Gameplay Screenshot 3"/>
  <img src="docs/screenshots/screenshot_20260219_214124.png" width="45%" alt="Gameplay Screenshot 4"/>
</div>

## ✨ Features

### 🎮 Gameplay
- **Endless forward-scrolling runner** on a single neon-lit platform
- **Jump** with coyote time and input buffering for forgiving controls
- **Dash** boost (ground only) with cooldown — earns style score and increases your multiplier
- **Dynamic difficulty** — forward speed and hazard probability ramp over time, capped at a configurable ceiling
- **Scoring** — distance score + style score combined with a speed-based multiplier (×1.00 – ×2.50)
- **Persistent leaderboard** — top 10 scores saved to a binary file with seeded defaults

### 🎨 Visuals
- **3 swappable color palettes** — Neon Dusk, Cyan Sunrise, Magenta Storm (cycle at runtime)
- **Optional bloom overlay** toggle
- **3D ship model** — Kenney Space Kit speeder (OBJ), with wireframe neon outline and engine exhaust trail
- **Rich visual scene dressing** — starfield with twinkling, mountain silhouettes, floating decorative cubes with glow halos, ambient particles, Tron-style grid, scrolling track bands, and speed streaks
- **Landing particle bursts** on touchdown
- **Dynamic follow camera** — smoothly tracks the ship, rolls on strafe, and widens FOV during speed boosts

### ⚡ Performance & Quality
- **Fixed timestep simulation** (1/120 s) decoupled from rendering with interpolation
- **Deterministic simulation** — Xorshift32 RNG, seeded runs are perfectly reproducible
- **14 automated simulation tests** via CTest (jump, dash, air control, scoring, difficulty, restart invariants, etc.)
- **Debug heap-allocation tracker** — overrides `operator new` in debug builds; warns when the update loop allocates
- **Performance overlay** — update/render milliseconds and allocation count shown on-screen
- **Cross-platform** — macOS, Linux, Windows; CMake `FetchContent` auto-downloads raylib 5.5

## 🛠️ Tech Stack

| Category | Technology |
|----------|-----------|
| **Language** | C++20 |
| **Graphics Library** | raylib 5.5 |
| **Build System** | CMake 3.21+ |
| **Testing Framework** | CTest |
| **Platforms** | Windows, macOS, Linux |
| **Compiler Support** | GCC 11+, Clang 14+, MSVC 2022+ |

## 🎮 Controls

### Gameplay

| Key | Action |
|-----|--------|
| **A** / **←** | Strafe left |
| **D** / **→** | Strafe right |
| **Space** | Jump (buffered; coyote time applies) |
| **Shift** | Dash forward (ground only, 0.5 s cooldown) |
| **R** | Restart same seed |
| **N** | New run (random seed) |
| **Tab** | Cycle color palette |
| **B** | Toggle bloom overlay |
| **Esc** / **P** | Pause |

### Menus

| Key | Action |
|-----|--------|
| **↑** / **W** | Move selection up |
| **↓** / **S** | Move selection down |
| **Enter** / **Space** | Confirm selection |
| **Esc** | Back / Exit |

### Game Over

| Key | Action |
|-----|--------|
| **R** | Retry same seed |
| **N** | New run |
| **Esc** / **Enter** | Return to main menu |

## 🚀 Quick Start

### Prerequisites

- ✅ CMake 3.21+
- ✅ C++20-capable compiler:
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Linux: GCC 11+ or Clang 14+
  - Windows: Visual Studio 2022 (MSVC) or Ninja + Clang/GCC
- ✅ Git

### macOS/Linux

```bash
# Clone the repository
git clone https://github.com/EyalShahaf/SkyRoadsDuplicate.git
cd SkyRoadsDuplicate

# Build and run
./scripts/build.sh
./scripts/run.sh
```

First build downloads raylib via CMake `FetchContent`, so it can take a bit.

**Run simulation tests:**
```bash
./scripts/test.sh
```

### Windows PowerShell

```powershell
# Clone the repository
git clone https://github.com/EyalShahaf/SkyRoadsDuplicate.git
cd SkyRoadsDuplicate

# Build and run
./scripts/build.ps1
./scripts/run.ps1
```

First build downloads raylib via CMake `FetchContent`, so it can take a bit.

**Run simulation tests:**
```powershell
./scripts/test.ps1
```

### Manual Build

#### macOS/Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/skyroads
```

#### Windows (Visual Studio generator)

```powershell
cmake -S . -B build
cmake --build build --config Release
./build/Release/skyroads.exe
```

## 🏗️ Architecture

```text
.
├── CMakeLists.txt          # Build config — fetches raylib, defines skyroads + sim_tests targets
├── core/                   # Foundation / infrastructure
│   ├── Config.hpp          #   Compile-time constants (physics, visuals, scoring, difficulty)
│   ├── Rng.hpp / .cpp      #   Deterministic Xorshift32 PRNG
│   ├── Assets.hpp / .cpp   #   Zero-alloc asset path resolver ("assets/<relative>")
│   └── PerfTracker.hpp/.cpp#   Debug-only heap allocation counter (operator new override)
├── game/                   # Game state & high-level logic
│   ├── Game.hpp            #   Central Game struct, screen enum, player/input/leaderboard types
│   └── Game.cpp            #   Init, input reading, meta actions, scoring, leaderboard I/O
├── sim/                    # Pure simulation (no rendering dependencies)
│   ├── Sim.hpp             #   SimStep() interface
│   └── Sim.cpp             #   Physics, jump/dash mechanics, scoring, difficulty ramp, particles
├── render/                 # All visual output
│   ├── Palette.hpp / .cpp  #   LevelPalette struct + 3 built-in palettes
│   └── Render.hpp / .cpp   #   Scene drawing, camera, HUD, exhaust particles, screen overlays
├── src/
│   └── main.cpp            #   Entry point — window init, fixed-timestep loop, perf measurement
├── tests/
│   └── SimTests.cpp        #   14 deterministic simulation tests (CTest)
├── assets/
│   └── models/             #   Kenney craft_speederA OBJ model
└── scripts/                #   Build / run / test helpers for macOS/Linux and Windows
```

### Key Design Decisions

| Aspect | Approach |
|--------|----------|
| **Sim / Render split** | `sim/` has zero rendering includes; can be tested headlessly via `sim_tests` |
| **Fixed timestep** | Simulation ticks at 120 Hz; rendering interpolates between previous and current state |
| **Zero-alloc update** | Preallocated particle pools + stack data; debug build warns on any heap allocation during update |
| **Deterministic replay** | Same seed → identical run; RNG state is explicit, never global |
| **Configuration** | All tuning constants live in `core/Config.hpp` as `constexpr` values |

## 📋 Requirements

- **CMake** 3.21+
- **C++20-capable compiler**:
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Linux: GCC 11+ or Clang 14+
  - Windows: Visual Studio 2022 (MSVC) or Ninja + Clang/GCC

## 🙏 Credits

- **Ship model** from [Kenney Space Kit](https://kenney.nl/assets/space-kit) (CC0)
- **Graphics library** [raylib](https://www.raylib.com/) 5.5
- **AI Development Assistance** - This project was developed with the help of AI coding assistants, demonstrating the collaborative potential of human-AI pair programming

---

<div align="center">

**⭐ If you enjoyed this project, consider giving it a star! ⭐**

</div>
