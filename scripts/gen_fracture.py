#!/usr/bin/env python3
#!/usr/bin/env python3
import sys


def gen_hypercube_dot(d, output):
    """
    Generates a DOT file for a perfect d-cube.
    """
    R = 2**d
    E = d * (2 ** (d - 1))
    with open(output, "w") as f:
        f.write(f"graph Hypercube_{d} {{\n")
        f.write(
            f'  graph [label="Optimal {d}-Cube (R={R}, E={E})", labelloc=t, fontname="Helvetica-bold", fontsize=20];\n'
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
    with open(output, "w") as f:
        f.write(f"graph Fracture_{d} {{\n")
        f.write(
            f'  graph [label="Fractured {d}-Cube (R={R}, E={E_max})", labelloc=t, fontname="Helvetica-bold", fontsize=20];\n'
        )
        f.write('  node [fontname="Helvetica", style=filled];\n')

        # Core: (R-1) subset of d-cube
        for i in range(R - 1):
            for bit in range(d):
                j = i ^ (1 << bit)
                if i < j and j < R - 1:
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

    d = int(sys.argv[1])
    prefix = sys.argv[2]

    gen_hypercube_dot(d, f"{prefix}_optimal.dot")
    gen_fracture_dot(d, f"{prefix}_fractured.dot")
    print(f"Generated {prefix}_optimal.dot and {prefix}_fractured.dot")
