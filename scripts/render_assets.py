#!/usr/bin/env python3
import sys
import subprocess
import os

def render_dots(asset_dir="assets"):
    """
    Renders all .dot files in the asset directory to .png using fdp.
    """
    if not os.path.exists(asset_dir):
        print(f"Error: {asset_dir} not found.")
        return

    dot_files = [f for f in os.listdir(asset_dir) if f.endswith(".dot")]
    if not dot_files:
        print(f"No .dot files found in {asset_dir}.")
        return

    print(f"Rendering {len(dot_files)} files from {asset_dir}...")
    for f in dot_files:
        input_path = os.path.join(asset_dir, f)
        output_path = os.path.join(asset_dir, f.replace(".dot", ".png"))

        # Using fdp for compact circular/force-directed layout
        cmd = ["fdp", "-Tpng", input_path, "-o", output_path]
        try:
            subprocess.run(cmd, check=True)
            print(f"  ✓ Rendered: {output_path}")
        except subprocess.CalledProcessError as e:
            print(f"  ✗ Failed: {input_path} ({e})")
        except FileNotFoundError:
            print("Error: 'fdp' command not found. Please install Graphviz.")
            sys.exit(1)

if __name__ == "__main__":
    render_dots()
