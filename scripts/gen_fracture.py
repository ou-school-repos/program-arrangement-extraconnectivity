#!/usr/bin/env python3
"""Generate DOT files for optimal and fractured hypercube graphs."""

import sys


def gen_hypercube_dot(d, output):
    """
    Generates a DOT file for a perfect d-cube.
    """
    R = 2**d
    E = d * (2 ** (d - 1))
    with open(output, "w", encoding="utf-8") as f:
        f.write(f"graph Hypercube_{d} {{\n")
        label = f'"Optimal {d}-Cube (R={R}, E={E})"'
        f.write(
            f"  graph [label={label},"
            f" labelloc=t,"
            f' fontname="Helvetica-bold",'
            f" fontsize=20];\n"
        )
        f.write('  node [fontname="Helvetica", style=filled, fillcolor=lightblue];\n')
        for i in range(R):
            for bit in range(d):
                j = i ^ (1 << bit)
                if i < j:
                    f.write(f"    {i} -- {j};\n")
        f.write("}\n")


def gen_fracture_dot(d, output):
    """
    Generates a DOT file for a fractured d-cube (2nd best topology).
    E_max = E_opt - d + 1
    """
    R = 2**d
    E_opt = d * (2 ** (d - 1))
    E_max = E_opt - d + 1
    with open(output, "w", encoding="utf-8") as f:
        f.write(f"graph Fracture_{d} {{\n")
        label = f'"Fractured {d}-Cube (R={R}, E={E_max})"'
        f.write(
            f"  graph [label={label},"
            f" labelloc=t,"
            f' fontname="Helvetica-bold",'
            f" fontsize=20];\n"
        )
        f.write('  node [fontname="Helvetica", style=filled];\n')

        # Core: (R-1) subset of d-cube
        for i in range(R - 1):
            for bit in range(d):
                j = i ^ (1 << bit)
                if i < j < R - 1:
                    f.write(f"    f{i} -- f{j};\n")

        # Splintered vertex (connected via only 1 edge to maintain connectivity)
        f.write(f'    f{R-1} [fillcolor=orange, label="N{R-1}\\n(Splintered)"];\n')
        f.write(
            f'    f0 -- f{R-1} [color=red, penwidth=2.0, label=" Rigidity Break"];\n'
        )
        f.write("}\n")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: gen_fracture.py [d] [output_prefix]")
        sys.exit(1)

    dimension = int(sys.argv[1])
    prefix = sys.argv[2]

    gen_hypercube_dot(dimension, f"{prefix}_optimal.dot")
    gen_fracture_dot(dimension, f"{prefix}_fractured.dot")
    print(f"Generated {prefix}_optimal.dot and {prefix}_fractured.dot")
