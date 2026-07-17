#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess
import shutil
import numpy as np
import uproot
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from matplotlib.ticker import MultipleLocator
from matplotlib import cm

def get_run_prefix(env_name="geant4"):
    """Returns the command prefix to run binaries/commands (e.g. micromamba run)."""
    if env_name in os.environ.get("CONDA_PREFIX", ""):
        return []
    if shutil.which("micromamba"):
        return ["micromamba", "run", "-n", env_name]
    elif shutil.which("conda"):
        return ["conda", "run", "-n", env_name]
    return []

def build_project(workspace_dir, env_name="geant4"):
    """Ensures the C++ project is configured and compiled."""
    build_dir = os.path.join(workspace_dir, "build")
    os.makedirs(build_dir, exist_ok=True)
    
    # Check if compiled binary exists
    binary_path = os.path.join(build_dir, "XFELVB")
    
    # We always run cmake and make to ensure any C++ changes are compiled
    print(">>> Configuring and compiling Geant4 C++ project...")
    try:
        # Run cmake with prefix path to micromamba environment to find EXPAT and other libs
        env_prefix = os.environ.get("CONDA_PREFIX", f"/Users/felip/.local/share/mamba/envs/{env_name}")
        prefix = get_run_prefix(env_name)
        # Find local CPU thread count for make if threads is not passed (use min of 4 or CPU count)
        build_threads = max(1, os.cpu_count() - 1) if os.cpu_count() else 4
        subprocess.run(
            prefix + ["cmake", f"-DCMAKE_PREFIX_PATH={env_prefix}", ".."],
            cwd=build_dir, check=True
        )
        # Compile using make
        subprocess.run(
            prefix + ["make", f"-j{build_threads}"],
            cwd=build_dir, check=True
        )
        print(">>> C++ project built successfully.")
    except subprocess.CalledProcessError as e:
        print(f"ERROR: Failed to compile Geant4 project: {e}")
        sys.exit(1)

def generate_macro(build_dir, kapton_um, air_m, primaries):
    """Generates a customized xray.mac macro file for the simulation."""
    macro_content = f"""
/control/verbose 0
/run/verbose 0

/testem/phys/addPhysics polarized

# Geometry parameters (PreInit state, before /run/initialize)
/testem/det/setKaptonThickness {kapton_um} um
/testem/det/setAirThickness {air_m} m

/run/initialize

/gun/polarization 0. 0. -1.
/gun/particle gamma
/gun/energy 8.766 keV

/analysis/setFileName polar
/analysis/h1/set 1  110  0     11 MeV	#gamma energy
/analysis/h1/set 2  100  -1.   1. 	#gamma cos(theta)
/analysis/h1/set 3  100  -180. 180. deg	#gamma phi
/analysis/h1/set 4  100  -1.5  1.5      #gamma polarization
/analysis/h1/set 5  110  0     11 MeV	#electron energy
/analysis/h1/set 6  100  -1.   1. 	#electron cos(theta)
/analysis/h1/set 7  100  -180. 180. deg	#electron phi
/analysis/h1/set 8  100  -1.5  1.5      #electron polarization
/analysis/h1/set 9  110  0     11 MeV	#positron energy
/analysis/h1/set 10 100  -1.   1. none	#positron cos(theta)
/analysis/h1/set 11 100  -180. 180. deg	#positron phi
/analysis/h1/set 12 100   -1.5  1.5     #positron polarization

# Print progress periodically
/run/printProgress {max(1, int(primaries // 100))}
/run/beamOn {int(primaries)}
"""
    # Write to build folder
    macro_path = os.path.join(build_dir, "xray.mac")
    with open(macro_path, "w") as f:
        f.write(macro_content.strip() + "\n")
        
    # Also copy/write to the workspace root directory so the user sees the updated parameters
    root_macro_path = os.path.join(os.path.dirname(build_dir), "xray.mac")
    try:
        with open(root_macro_path, "w") as f:
            f.write(macro_content.strip() + "\n")
    except Exception as e:
        print(f"WARNING: Could not update root xray.mac: {e}")
        
    print(f">>> Generated macro: {macro_path} and {root_macro_path}")
    return macro_path

def run_simulation(build_dir, threads, primaries, env_name="geant4"):
    """Runs the Geant4 simulation binary with the macro and thread arguments."""
    binary = "./XFELVB"
    macro = "xray.mac"
    print(f">>> Running simulation: {binary} {macro} {threads} (Cwd: {build_dir})")
    try:
        prefix = get_run_prefix(env_name)
        
        # Start the process with output piped
        process = subprocess.Popen(
            prefix + [binary, macro, str(threads)],
            cwd=build_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        
        import re
        event_pattern = re.compile(r"--> Event\s+(\d+)\s+starts")
        
        # Simple text progress bar function
        def print_progress_bar(completed, total):
            percent = (completed / total) * 100.0
            filled_length = int(50 * completed // total)
            bar = '█' * filled_length + '-' * (50 - filled_length)
            sys.stdout.write(f"\rProgress: |{bar}| {percent:.1f}% ({completed:,}/{total:,})")
            sys.stdout.flush()
            
        print_progress_bar(0, primaries)
        
        for line in process.stdout:
            match = event_pattern.search(line)
            if match:
                completed = int(match.group(1))
                print_progress_bar(completed, primaries)
                
        process.wait()
        print_progress_bar(primaries, primaries)
        print("\n>>> Simulation run completed successfully.")
        
        if process.returncode != 0:
            raise subprocess.CalledProcessError(process.returncode, prefix + [binary, macro, str(threads)])
            
    except subprocess.CalledProcessError as e:
        print(f"\nERROR: Simulation failed: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\nERROR: Running simulation failed: {e}")
        sys.exit(1)

def analyze_results(root_file, output_dir, save_basename, kapton_um, air_m, primaries):
    """Processes ROOT output, exports hits for all particle types, generates plots, and writes a report."""
    print(f">>> Analyzing ROOT file: {root_file}")
    if not os.path.exists(root_file):
        print(f"ERROR: ROOT file not found: {root_file}")
        sys.exit(1)
        
    # Open the ROOT file
    file = uproot.open(root_file)
    
    # The ntuple is named 'Gamma' but stores ALL particle types
    if "Gamma;1" not in file:
        print("ERROR: 'Gamma;1' tree not found in ROOT file. Available keys:", file.keys())
        sys.exit(1)
        
    hits_tree = file["Gamma;1"]
    
    # Load all columns
    x        = hits_tree["x"].array(library="np")       # µm
    y        = hits_tree["y"].array(library="np")       # µm
    energy   = hits_tree["energy"].array(library="np")  # keV
    particle = hits_tree["particle"].array(library="np") # 1=gamma, 5=electron, 9=positron

    # Particle IDs
    GAMMA_ID    = 1
    ELECTRON_ID = 5
    POSITRON_ID = 9

    # Per-type masks
    gamma_mask    = particle == GAMMA_ID
    electron_mask = particle == ELECTRON_ID
    positron_mask = particle == POSITRON_ID

    n_gamma    = int(np.sum(gamma_mask))
    n_electron = int(np.sum(electron_mask))
    n_positron = int(np.sum(positron_mask))
    n_total    = len(x)

    # Save all hits (all types) to the npz file
    npz_path = os.path.join(output_dir, f"{save_basename}.npz")
    np.savez_compressed(npz_path, x=x, y=y, e=energy, particle=particle)
    print(f">>> Saved compressed hits to: {npz_path}")
    print(f"    Total hits on screen: {n_total:,}  (gamma: {n_gamma:,}, e-: {n_electron:,}, e+: {n_positron:,})")
    
    # Statistics: use CLI --primaries as source of truth for primaries count
    # (Summary ntuple merging is unreliable for integer columns across threads)
    total_primaries = float(primaries)
    transmission_pct = (n_gamma / total_primaries) * 100.0  # gamma transmission w.r.t. beam

    # Plotting: 3 subplots — 2D spatial (all), energy spectrum (all types stacked)
    print(">>> Generating plots...")
    fig, axes = plt.subplots(1, 2, figsize=(18, 6))

    # --- Plot 1: 2D Spatial Distribution (all particles) ---
    ax0 = axes[0]
    if n_gamma > 0:
        h = ax0.hist2d(
            x[gamma_mask], y[gamma_mask], bins=100,
            range=[[-300, 300], [-300, 300]],
            cmap="viridis", norm=LogNorm()
        )
        fig.colorbar(h[3], ax=ax0, label="Counts (γ only)")
    ax0.xaxis.set_major_locator(MultipleLocator(100))
    ax0.xaxis.set_minor_locator(MultipleLocator(50))
    ax0.yaxis.set_major_locator(MultipleLocator(100))
    ax0.yaxis.set_minor_locator(MultipleLocator(50))
    ax0.set_ylabel("y [µm]")
    ax0.set_xlabel("x [µm]")
    ax0.xaxis.grid(which="major", alpha=1)
    ax0.xaxis.grid(which="minor", alpha=0.3)
    ax0.yaxis.grid(which="major", alpha=1)
    ax0.yaxis.grid(which="minor", alpha=0.3)
    ax0.set_ylim(-300, 300)
    ax0.set_xlim(-300, 300)
    ax0.set_title(f"γ Screen Hits (Kapton: {kapton_um}µm, Air: {air_m}m)")

    # --- Plot 2: Energy Spectrum per particle type ---
    ax1 = axes[1]
    hist_kwargs = dict(bins=120, range=(0, 12), histtype='step', linewidth=1.5)
    if n_gamma > 0:
        ax1.hist(energy[gamma_mask],    label=f"γ (N={n_gamma:,})",    color='royalblue',  **hist_kwargs)
    if n_electron > 0:
        ax1.hist(energy[electron_mask], label=f"e⁻ (N={n_electron:,})", color='tomato',     **hist_kwargs)
    if n_positron > 0:
        ax1.hist(energy[positron_mask], label=f"e⁺ (N={n_positron:,})", color='mediumseagreen', **hist_kwargs)
    ax1.set_xlabel("Energy [keV]")
    ax1.set_yscale('log')
    ax1.set_ylabel("Particles")
    ax1.set_title("Energy Spectrum by Particle Type")
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    fig.tight_layout()

    # Save plots as png and pdf
    png_path = os.path.join(output_dir, f"{save_basename}.png")
    pdf_path = os.path.join(output_dir, f"{save_basename}.pdf")
    fig.savefig(png_path, bbox_inches="tight", dpi=300)
    fig.savefig(pdf_path, bbox_inches="tight", dpi=300)
    plt.close(fig)
    print(f">>> Saved plots: {png_path} and {pdf_path}")
    
    # Write summary markdown report
    summary_path = os.path.join(output_dir, "summary.md")
    write_summary_report(
        summary_path, kapton_um, air_m, primaries,
        n_gamma, n_electron, n_positron, transmission_pct
    )
    
    # Print results to console
    print("\n" + "="*50)
    print(f"Simulation Analysis Summary")
    print("="*50)
    print(f"Kapton Thickness  : {kapton_um} um")
    print(f"Air Gap Thickness : {air_m} m")
    print(f"Primary Gammas    : {primaries:,.0f}")
    print(f"  γ  Screen Hits  : {n_gamma:,}")
    print(f"  e- Screen Hits  : {n_electron:,}")
    print(f"  e+ Screen Hits  : {n_positron:,}")
    print(f"γ Transmission    : {transmission_pct:.5f} %")
    print(f"Results Saved In  : {output_dir}")
    print("="*50 + "\n")

def write_summary_report(file_path, kapton_um, air_m, primaries,
                         n_gamma, n_electron, n_positron, transmission):
    """Writes a professional, clean markdown summary report with per-particle statistics."""
    n_total = n_gamma + n_electron + n_positron
    content = f"""# X-ray Transmission Simulation Summary

This simulation calculates the transmission of {primaries:,.0f} primary X-ray gammas ($8.766\\text{{ keV}}$) through a Kapton window of {kapton_um} $\\mu\\text{{m}}$ thickness followed by an Air gap of {air_m} $\\text{{m}}$ thickness.

## Parameters

| Parameter | Value |
| :--- | :--- |
| **Primary Particle** | Gamma ($\\gamma$) |
| **Primary Energy** | $8.766\\text{{ keV}}$ |
| **Primary Beam Count** | {primaries:,.0f} |
| **Kapton Thickness** | {kapton_um} $\\mu\\text{{m}}$ |
| **Air Volume Thickness** | {air_m} $\\text{{m}}$ |

## Screen Hit Results

| Particle | Screen Hits |
| :--- | ---: |
| **γ (Gamma)** | {n_gamma:,} |
| **e⁻ (Electron)** | {n_electron:,} |
| **e⁺ (Positron)** | {n_positron:,} |
| **Total** | {n_total:,} |

*   **γ Transmission Rate:** **`{transmission:.5f}%`**

## Remarks

The gamma transmission represents the fraction of primary $8.766\\text{{ keV}}$ X-ray photons
that successfully traverse the Kapton window and Air gap and are recorded on the screen.
Secondary electrons and positrons are produced by photoelectric absorption and Compton
scattering in the Kapton and Air materials.
All plots, data arrays (`.npz`), and simulation configurations are stored in the same folder.
"""
    with open(file_path, "w") as f:
        f.write(content.strip() + "\n")
    print(f">>> Saved summary report: {file_path}")


def main():
    parser = argparse.ArgumentParser(description="Run and analyze Geant4 X-ray simulation.")
    parser.add_argument("--kapton", type=float, default=50.0, help="Kapton thickness in micrometers (default: 50.0)")
    parser.add_argument("--air", type=float, default=1.0, help="Air volume thickness in meters (default: 1.0)")
    parser.add_argument("--threads", type=int, default=5, help="Number of threads (default: 5)")
    parser.add_argument("--primaries", type=float, default=2.1e9, help="Number of primary particles (default: 2.1e9)")
    parser.add_argument("--output-dir", type=str, default=None, help="Custom output directory")
    parser.add_argument("--env", type=str, default="geant4", help="Conda/Micromamba environment name (default: geant4)")
    
    args = parser.parse_args()
    
    workspace_dir = os.path.abspath(os.path.dirname(__file__))
    build_dir = os.path.join(workspace_dir, "build")
    
    # 1. Compile project
    build_project(workspace_dir, args.env)
    
    # 2. Setup output folder and file naming
    if args.output_dir:
        output_dir = os.path.abspath(args.output_dir)
        save_basename = "results"
    else:
        # Default name format using thicknesses
        folder_name = f"results_{args.kapton}um_{args.air}m"
        output_dir = os.path.join(workspace_dir, "results", folder_name)
        save_basename = f"xray_Primaries{args.primaries:.1e}_{args.kapton}umKapton_{args.air}mAir"
        
    os.makedirs(output_dir, exist_ok=True)
    
    # 3. Generate xray.mac macro file
    generate_macro(build_dir, args.kapton, args.air, args.primaries)
    
    # 4. Run the Geant4 simulation
    run_simulation(build_dir, args.threads, args.primaries, args.env)
    
    # 5. Locate and process the ROOT output
    root_file = os.path.join(build_dir, "results.root")
    analyze_results(root_file, output_dir, save_basename, args.kapton, args.air, args.primaries)

if __name__ == "__main__":
    main()
