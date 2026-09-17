#!/usr/bin/env python3
"""Render the full-Star validator regime table as a categorical heatmap."""

from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.colors import BoundaryNorm, ListedColormap

LABELS = {
    "I": 0,
    "S": 1,
    "s": 2,
    "H": 3,
    "--": -1,
}

ROWS = {
    8: ["I", "I", "S", "S", "S", "S", "S", "--"],
    9: ["I", "I", "S", "S", "S", "s", "S", "S"],
    10: ["I", "I", "S", "S", "S", "s", "s", "s"],
    11: ["I", "I", "S", "S", "S", "H", "s", "s"],
    12: ["I", "I", "S", "S", "S", "H", "s", "s"],
    13: ["I", "I", "S", "S", "S", "H", "H", "s"],
}


def main() -> None:
    """Write the validator regime heatmap."""
    labels = list(ROWS.values())
    values = [[LABELS[label] for label in row] for row in labels]
    cmap = ListedColormap(["#cccccc", "#4c72b0", "#dd8452", "#c44e52"])
    norm = BoundaryNorm([-0.5, 0.5, 1.5, 2.5, 3.5], cmap.N)

    figure, axis = plt.subplots(figsize=(8, 5))
    image = axis.imshow(values, cmap=cmap, norm=norm, aspect="auto")
    del image
    axis.set_xticks(range(8), range(1, 9))
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
            ("#cccccc", "I: invalid extra cut"),
            ("#4c72b0", "S: Hamming-safe Star"),
            ("#dd8452", "s: soft counterexample"),
            ("#c44e52", "H: hard counterexample"),
        ]
    ]
    axis.legend(handles=handles, loc="upper left", bbox_to_anchor=(1.02, 1))
    figure.tight_layout()

    output = Path("docs/star-validator-heatmap.png")
    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=300)
    print(f"Heatmap saved to {output}")


if __name__ == "__main__":
    main()
