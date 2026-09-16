#!/usr/bin/env python3
"""Plot the exact -8 averaged-squeeze failure for the A(8,4) full Star."""

from pathlib import Path

import matplotlib.pyplot as plt


def main():
    """Write a PDF and PNG comparison of the exact cost and budget."""
    budget = 676
    child_potential = 460
    collision_cost = 144
    defect_cost = 80
    exact_cost = child_potential + collision_cost + defect_cost

    figure, axis = plt.subplots(figsize=(7.2, 3.8))
    axis.barh([0], [budget], color="#d9e8f5", edgecolor="#234e70", height=0.48,
              label=r"Arithmetic budget $4P(17)=676$")
    left = 0
    components = [
        (child_potential, "child P", "#4c78a8"),
        (collision_cost, r"$\sum\Delta X$", "#f58518"),
        (defect_cost, r"$5D$", "#e45756"),
    ]
    for width, label, color in components:
        axis.barh([1], [width], left=left, color=color, edgecolor="white",
                  height=0.48, label=f"{label} = {width}")
        axis.text(left + width / 2, 1, str(width), ha="center", va="center",
                  color="white", fontweight="bold")
        left += width

    axis.axvline(budget, color="#222222", linestyle="--", linewidth=1.4)
    axis.annotate(
        r"exact cost $684$  ($+8$ over budget)",
        xy=(exact_cost, 1), xytext=(budget - 90, 1.42),
        arrowprops={"arrowstyle": "-|>", "color": "#222222"},
        ha="center", va="bottom",
    )
    axis.text((budget + exact_cost) / 2, 0.32, r"slack $=676-684=-8$",
              ha="center", va="center", color="#b2182b", fontweight="bold")
    axis.set_yticks([0, 1], ["budget", "full Star cost"])
    axis.set_xlim(0, 735)
    axis.set_xlabel("units")
    axis.set_title(r"Coordinate-averaged squeeze failure: full Star $S_4\subset A(8,4)$")
    axis.grid(axis="x", alpha=0.2)
    axis.legend(loc="upper left", bbox_to_anchor=(0, -0.22), ncol=2,
                frameon=False)
    figure.tight_layout()

    output_dir = Path("docs/figures")
    figure.savefig(output_dir / "star_averaged_squeeze_failure.pdf")
    figure.savefig(output_dir / "star_averaged_squeeze_failure.png", dpi=220)
    plt.close(figure)


if __name__ == "__main__":
    main()
