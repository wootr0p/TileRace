# TileRace

**Prototype of a Vertical(?) Racing Multiplayer 2D Platform** *with potential cross-platform support.*

---

## Prerequisites

Before cloning the project, ensure you have **Visual Studio 2026** installed with the following workload:

* **Desktop development with C++**

---

## Build Instructions

### 1. Clone the Repository

Open your terminal in your preferred workspace directory and run:

```bash
git clone https://github.com/wootr0p/TileRace.git

```

### 2. Open the Project

Locate the `TileRace.slnx` file in the root directory and open it with Visual Studio.

### 3. Mandatory First-Time Setup ⚠️

Due to the project's security and cleanup policies (`.gitignore`), local IDE settings are not tracked. **You must perform these two steps manually** the first time you open the solution:

#### A. Set the Startup Project

1. In the **Solution Explorer**, find the **TileRace** project.
2. Right-click on it and select **Set as Startup Project**. (The project name should now appear in **bold**).

#### B. Configure the Working Directory

To ensure the game can find textures, sounds, and assets:

1. Right-click on the **TileRace** project > **Properties**.
2. Go to **Configuration Properties** > **Debugging**.
3. Locate the **Working Directory** field.
4. Set it to: `$(SolutionDir)` (or the specific folder where your assets are located).
5. Click **Apply** and **OK**.

### 4. Build and Launch

1. Set the build configuration to **Debug** or **Release** in the top toolbar.
2. Press **F5** or click the **Start** button to compile and run.
