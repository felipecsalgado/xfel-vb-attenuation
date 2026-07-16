# X-ray Transmission Simulation (GEANT4)

This Geant4 simulation project calculates the transmission of primary polarized X-ray gammas (8.8 keV) through a variable thickness Kapton window followed by a variable thickness Air gap. It records the positions, momenta, and energy of the particles that cross into a downstream screen detector.

## Project Structure

*   `Pol01.cc`: Main program file initializing the multi-threaded run manager, physics list, and action classes.
*   `RunAndAnalyze.py`: Python automation runner. It configures and compiles the project, generates the macro file, executes the Geant4 simulation, analyzes the ROOT ntuples, and outputs plots, binary data, and markdown summaries.
*   `src/`: C++ source implementations for geometry construction, stepping action, and tracking/run management.
*   `include/`: C++ headers matching the source definitions.
*   `CMakeLists.txt`: Build system configuration.
*   `results/`: Directory containing folders with validation and execution results (matplotlib plots, `.npz` arrays, and summary reports).

---

## Command Line Parameters and Options

### Python Execution Wrapper (`RunAndAnalyze.py`)

The python script simplifies compiling, macro creation, simulation execution, and data analysis.

```bash
python RunAndAnalyze.py [options]
```

#### Available Parameters:
*   `--kapton <value>`: Kapton window thickness in micrometers (default: `50.0`). Set to `0` to omit the Kapton window from the geometry.
*   `--air <value>`: Air gap volume thickness in meters (default: `1.0`).
*   `--threads <value>`: Number of threads to use in Geant4 multithreading mode (default: total logical CPU cores).
*   `--primaries <value>`: Number of primary gamma particles to simulate (default: `2.1e9`). Supports scientific notation (e.g., `1e5`).
*   `--skip-sim`: Run data analysis and plot generation on the existing `compton.root` file without triggering a new Geant4 run.

### Geant4 C++ Macro Commands

You can configure geometry and beam parameters dynamically using Geant4 messenger UI commands (placed in macro files like `xray.mac` before `/run/initialize`):

*   `/testem/det/setKaptonThickness <val> <unit>`: Set Kapton window thickness (e.g., `50.0 um`).
*   `/testem/det/setAirThickness <val> <unit>`: Set Air gap thickness (e.g., `1.0 m`).
*   `/testem/det/setSizeXY <val> <unit>`: Set the transverse size of the volumes.
*   `/testem/det/update`: Rebuild and update the detector geometry.

---

## Installation, Compilation and Execution

### Prerequisites

*   Geant4 installation (configured with multi-threading and ROOT format support).
*   C++ Compiler (Clang or GCC with C++17 support) and CMake (version 3.10 or higher).
*   Python 3 environment with package requirements: `uproot`, `numpy`, `matplotlib`.

### Setup Environment (Example using Micromamba/Conda)

To install Python dependencies in your conda/micromamba environment:
```bash
micromamba install -n geant4 python uproot numpy matplotlib
```

### Option A: Complete Automation (Recommended)

To compile the C++ codebase, generate a macro, run the simulation, and produce the analysis plots in one command:
```bash
python RunAndAnalyze.py --kapton 50 --air 0.4 --threads 5 --primaries 1000000
```
This script handles building the workspace under a `build/` directory, invoking the executable, and outputting plots and results to a dedicated directory named after the configuration (e.g., `results/results_50.0um_0.4m/`).

### Option B: Manual Compilation and Execution

#### 1. Configure and Build the Project
Create a build directory, run CMake, and compile the source code:
```bash
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

#### 2. Run the C++ Binary
Execute the simulation binary with a macro file and the number of threads:
```bash
./Pol01 xray.mac 4
```
Where `xray.mac` is your configuration macro and `4` is the number of threads.
