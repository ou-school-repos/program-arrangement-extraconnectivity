#!/usr/bin/env python3
"""Render the full-Star validator regime table as a categorical heatmap."""

from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.colors import BoundaryNorm, ListedColormap

LABELS = {
    "—": 0,
    "✓": 1,
    "□": 2,
    "■": 3,
    "/": 4,
    "…": 5,
}

ROWS = {
    8: ["—", "—", "✓", "✓", "✓", "✓", "✓", "—", "—", "—"],
    9: ["—", "—", "✓", "✓", "✓", "□", "✓", "✓", "—", "—"],
    10: ["—", "—", "✓", "✓", "✓", "□", "□", "□", "✓", "—"],
    11: ["—", "—", "✓", "✓", "✓", "■", "□", "□", "□", "/"],
    12: ["—", "—", "✓", "✓", "✓", "■", "□", "□", "/", "/"],
    13: ["—", "—", "✓", "✓", "✓", "■", "■", "□", "/", "/"],
    14: ["—", "—", "✓", "✓", "✓", "■", "■", "■", "/", "/"],
    15: ["—", "—", "✓", "✓", "✓", "■", "■", "■", "/", "/"],
    16: ["—", "—", "✓", "✓", "✓", "■", "■", "■", "/", "/"],
    17: ["—", "—", "✓", "✓", "✓", "□", "■", "/", "/", "/"],
    18: ["—", "—", "✓", "✓", "✓", "□", "■", "/", "/", "/"],
    19: ["—", "—", "✓", "✓", "✓", "□", "■", "/", "/", "/"],
    20: ["—", "—", "✓", "✓", "✓", "□", "■", "/", "/", "/"],
}


def main() -> None:
    """Write the validator regime heatmap."""
    labels = list(ROWS.values())
    values = [[LABELS[label] for label in row] for row in labels]
    cmap = ListedColormap(
        [
            "#cccccc",  # invalid or outside domain
            "#4c72b0",  # Hamming-safe
            "#dd8452",  # soft
            "#c44e52",  # hard
            "#d9d9d9",  # guard rejected
            "#ffffff",  # not run
        ]
    )
    norm = BoundaryNorm([-0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5], cmap.N)

    figure, axis = plt.subplots(figsize=(8, 5))
    image = axis.imshow(values, cmap=cmap, norm=norm, aspect="auto")
    del image
    axis.set_xticks(range(10), range(1, 11))
    axis.set_yticks(range(len(ROWS)), ROWS.keys())
    axis.set_xlabel("k")
    axis.set_ylabel("n")
    axis.set_title("Full-Star validator regimes")

    for row_index, row in enumerate(labels):
        for column_index, label in enumerate(row):
            axis.text(column_index, row_index, label, ha="center", va="center")

    handles = [
        plt.Rectangle((0, 0), 1, 1, color=color, label=label)
        for color, label in [
            ("#cccccc", "—: invalid or outside k < n"),
            ("#4c72b0", "✓: satisfies Hamming comparison"),
            ("#dd8452", "□: soft counterexample"),
            ("#c44e52", "■: hard counterexample"),
            ("#d9d9d9", "/: rejected by guard"),
            ("#ffffff", "…: not yet run"),
        ]
    ]
    axis.legend(handles=handles, loc="upper left", bbox_to_anchor=(1.02, 1))
    figure.tight_layout()

    output = Path("assets/out/star-validator-heatmap.png")
    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=300)
    print(f"Heatmap saved to {output}")


if __name__ == "__main__":
    main()
