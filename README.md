# TileRace

**Competitive multiplayer 2D side-scrolling platformer.** Two or more players race across tile-based levels to reach the goal first. Supports both online (dedicated server) and offline (embedded local server) modes. Up to 8 players per match.

---

## Build Instructions

This project uses **CMake 3.16+** with CMake Presets and Ninja. All dependencies (Raylib 5.5, ENet) are fetched and compiled from source via `FetchContent` — no pre-built binaries required.

### Prerequisites

- CMake 3.16 or higher
- A C++20-compatible compiler (MSVC, GCC, or Clang)
- Ninja build system (recommended)

### First-time configure

```bash
# Debug
cmake --preset debug

# Release
cmake --preset release
```

Only needs to be run once (or after changing `CMakeLists.txt` / `CMakePresets.json`).

### Build & Run

| Command | Effect |
|---|---|
| `cmake --build --preset run-debug` | Build (debug) → launch client |
| `cmake --build --preset run-release` | Build (release) → launch client |
| `cmake --build --preset run-scc-debug` | Build (debug) → launch server + 2 clients |
| `cmake --build --preset run-scc-release` | Build (release) → launch server + 2 clients |
| `cmake --build --preset debug` | Build only (debug) |
| `cmake --build --preset release` | Build only (release) |

The `run-scc-*` presets open the server and each client in separate windows via PowerShell `Start-Process`. The server starts first; clients follow after a 1-second delay.

### Create a Distributable Build

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --component TileRaceRuntime
```

Output is placed in `dist/` at the project root and contains exactly what is needed to distribute the game.

