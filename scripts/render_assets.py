#!/usr/bin/env python3
import os
import subprocess


def to_bin(i, d):
    return bin(i)[2:].zfill(d)


def gen_comparison_dot(d, output):
    """
    Generates a single DOT file with two clusters (side-by-side)
    comparing the optimal and fractured topologies.
    """
    R = 2**d
    E_opt = d * (2 ** (d - 1))
    E_max = E_opt - d + 1

    with open(output, "w") as f:
        f.write(f"graph Comparison_{d} {{\n")
        label = f'"Stability Analysis (R={R})' f'\\nOptimal vs Fractured"'
        f.write(
            f"  graph [label={label},"
            f" labelloc=t,"
            f' fontname="Helvetica-bold",'
            f" fontsize=20];\n"
        )
        f.write(
            '  node [fontname="Helvetica",'
            " style=filled, shape=circle,"
            " width=0.6];\n"
        )
        f.write("  edge [penwidth=1.2];\n")

        # Optimal Cluster
        f.write("  subgraph cluster_opt {\n")
        f.write(f'    label="Optimal {d}-Cube (E={E_opt})";\n')
        f.write("    color=blue; fontcolor=blue; style=dashed;\n")
        for i in range(R):
            f.write(f'    o{i} [fillcolor=lightblue, label="{to_bin(i, d)}"];\n')
        for i in range(R):
            for bit in range(d):
                j = i ^ (1 << bit)
                if i < j:
                    f.write(f"    o{i} -- o{j};\n")
        f.write("  }\n")

        # Fractured Cluster
        f.write("  subgraph cluster_frac {\n")
        f.write(f'    label="Fractured {d}-Cube (E={E_max})";\n')
        f.write("    color=red; fontcolor=red; style=dashed;\n")
        for i in range(R - 1):
            f.write(f'    f{i} [fillcolor=lightpink, label="{to_bin(i, d)}"];\n')
        for i in range(R - 1):
            for bit in range(d):
                j = i ^ (1 << bit)
                if i < j and j < R - 1:
                    f.write(f"    f{i} -- f{j};\n")

        # The Fractured/Splintered Node
        lbl = to_bin(R - 1, d)
        f.write(f"    f{R-1} [fillcolor=orange," f' label="{lbl}\n(Splintered)"];\n')
        f.write(
            f"    f0 -- f{R-1} [color=red,"
            f" penwidth=3.0,"
            f' label="Rigidity\nBreak"];\n'
        )
        f.write("  }\n")
        f.write("}\n")


def render_dots(asset_dir="assets", output_dir="assets/out"):
    """
    Renders all .dot files in the asset directory to .png using fdp.
    """
    if not os.path.exists(asset_dir):
        print(f"Error: {asset_dir} not found.")
        return
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    dot_files = [f for f in os.listdir(asset_dir) if f.endswith(".dot")]
    if not dot_files:
        print(f"No .dot files found in {asset_dir}.")
        return

    print(f"Rendering {len(dot_files)} files to {output_dir}...")
    for f in dot_files:
        input_path = os.path.join(asset_dir, f)
        output_path = os.path.join(output_dir, f.replace(".dot", ".png"))

        # Using GIF for lighter weight, and scale down DPI to prevent high-res
        cmd = ["fdp", "-Tpng", "-Gdpi=60", input_path, "-o", output_path]
        try:
            subprocess.run(cmd, check=True)
            print(f"  ✓ Rendered: {output_path}")
        except subprocess.CalledProcessError as e:
            print(f"  ✗ Failed: {input_path} ({e})")


if __name__ == "__main__":
    # Generate side-by-side comparison DOTs natively
    gen_comparison_dot(3, "assets/r8_stability.dot")
    gen_comparison_dot(4, "assets/r16_stability.dot")
    print("Generated stability comparison DOTs natively.")

    # Render all assets
    render_dots()
