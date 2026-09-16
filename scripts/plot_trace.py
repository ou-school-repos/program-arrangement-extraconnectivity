#!/usr/bin/env python3
"""Plot the ordering-dependent cumulative boundary diagnostic."""

from pathlib import Path

import matplotlib.pyplot as plt


def main():
    """Generate and save the cumulative boundary comparison plot."""
    steps = list(range(1, 27))
    binary_slice = [
        25,
        44,
        62,
        76,
        93,
        106,
        119,
        135,
        147,
        159,
        171,
        180,
        188,
        203,
        214,
        225,
        234,
        245,
        252,
        259,
        270,
        277,
        284,
        291,
        295,
        298,
    ]
    full_star = [
        25,
        44,
        63,
        82,
        101,
        120,
        135,
        150,
        165,
        180,
        195,
        206,
        217,
        228,
        239,
        250,
        257,
        264,
        271,
        278,
        285,
        288,
        291,
        294,
        297,
        300,
    ]

    figure, axis = plt.subplots(figsize=(10, 6))
    axis.plot(
        steps,
        full_star,
        marker="o",
        color="#d62728",
        linewidth=2,
        label="Full Star (300)",
    )
    axis.plot(
        steps,
        binary_slice,
        marker="s",
        color="#1f77b4",
        linewidth=2,
        label="Binary slice (298)",
    )
    axis.fill_between(steps, binary_slice, full_star, color="gray", alpha=0.15)
    axis.set_title("Cumulative Vertex Boundary Growth in A(10,5)")
    axis.set_xlabel("Subset size")
    axis.set_ylabel(r"Cumulative boundary $|\partial V|$")
    axis.legend(loc="upper left")
    axis.grid(True, linestyle="--", alpha=0.6)
    figure.text(
        0.5,
        0.01,
        "Diagnostic under lexicographic insertion ordering; step increments "
        "are ordering-dependent and are not a universal bound.",
        ha="center",
        fontsize=9,
        style="italic",
    )
    figure.subplots_adjust(bottom=0.16)
    output_dir = Path("assets/out")
    output_dir.mkdir(parents=True, exist_ok=True)
    png_path = output_dir / "boundary_diagnostic_plot.png"
    pdf_path = output_dir / "boundary_diagnostic_plot.pdf"
    figure.savefig(png_path, dpi=300)
    figure.savefig(pdf_path)
    print(f"Plots saved to {png_path} and {pdf_path}")


if __name__ == "__main__":
    main()
