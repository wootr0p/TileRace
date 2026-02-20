# TileRace

**Prototype of a Vertical(?) Racing Multiplayer 2D Platform.** *With potential cross-platform support.*

## Prerequisites

Before cloning the project, ensure you have **Visual Studio 2026** (or later) installed with the following workload:

* **Desktop development with C++**

## Build Instructions

Follow these steps to set up your local development environment:

### 1. Clone the Repository

Open your terminal or Git Bash in your preferred workspace directory and run:

```bash
git clone https://github.com/wootr0p/TileRace.git

```

### 2. Open the Project

Locate the `TileRace.slnx` file in the root directory and open it with Visual Studio.

### 3. Build and Run

1. **Select Configuration:** Set the build configuration to `Debug` or `Release` (top toolbar).
2. **Set Startup Project:** Since local user settings are not tracked by Git, if the project is not already highlighted in bold, right-click on the **TileRace** project in the *Solution Explorer* and select **Set as Startup Project**.
3. **Launch:** Press `F5` or the **Start** button to compile and run the application.

---

## Technical Notes

* **Working Directory:** The project is configured to look for assets relative to the solution directory. If you encounter issues loading textures or sounds, verify the *Working Directory* in `Project Properties > Debugging`.

