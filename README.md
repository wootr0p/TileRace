# TileRace

**Prototype of a Vertical(?) Racing Multiplayer 2D Platform** _with potential cross-platform support._

---

## Build Instructions

This project uses CMake and CMake Presets for a cross-platform and IDE-independent build process.

### Prerequisites

- CMake (3.21 or higher)
- A C++17 compatible compiler (MSVC, GCC, or Clang)
- Ninja build system (recommended)

### Build and Run

You can automatically build and launch both the Server and the Client with a single command using the provided presets.

**Debug Mode:**

```bash
cmake --preset debug
cmake --build --preset run-debug
```

**Release Mode:**

```bash
cmake --preset release
cmake --build --preset run-release
```

### Create a Distributable Build

To create a clean `dist` folder containing only the files needed to distribute the game (executables, shared libraries, and assets), run the following commands:

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --component TileRaceRuntime
```

The output will be located in the `dist/` directory at the root of the project.
