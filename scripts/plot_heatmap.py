#!/usr/bin/env python3
"""Render the full-Star validator regime table as a categorical heatmap."""

from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.colors import BoundaryNorm, ListedColormap

LABELS = {
    "/": 0,
    "o": 1,
    "x": 2,
    "X": 3,
    "~": 4,
    "?": 5,
    ".": 6,
}

ROWS = {
    8: ["/", "/", "o", "o", "o", "o", "o", "~", "~", "~"],
    9: ["/", "/", "o", "o", "o", "x", "o", "o", "~", "~"],
    10: ["/", "/", "o", "o", "o", "x", "x", "x", "o", "~"],
    11: ["/", "/", "o", "o", "o", "X", "x", "x", "?", "?"],
    12: ["/", "/", "o", "o", "o", "X", "x", "x", "?", "?"],
    13: ["/", "/", "o", "o", "o", "X", "X", "x", "?", "?"],
    14: ["/", "/", "o", "o", "o", "X", "X", "X", "?", "?"],
    15: ["/", "/", "o", "o", "o", "X", "X", "?", "?", "?"],
    16: ["/", "/", "o", "o", "o", "X", "X", "?", "?", "?"],
    17: ["/", "/", "o", "o", "o", "x", "X", "?", "?", "?"],
    18: ["/", "/", "o", "o", "o", "x", "X", "?", "?", "?"],
    19: ["/", "/", "o", "o", "o", "x", "X", "?", "?", "?"],
    20: ["/", "/", "o", "o", "o", "x", "X", "?", "?", "?"],
}


def main() -> None:
    """Write the validator regime heatmap."""
    labels = list(ROWS.values())
    values = [[LABELS[label] for label in row] for row in labels]
    cmap = ListedColormap(
        [
            "#cccccc",  # invalid
            "#4c72b0",  # Hamming-safe
            "#dd8452",  # soft
            "#c44e52",  # hard
            "#f2f2f2",  # irrelevant
            "#d9d9d9",  # guard rejected
            "#ffffff",  # not run
        ]
    )
    norm = BoundaryNorm([-0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5], cmap.N)

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
            ("#cccccc", "/: invalid extra cut"),
            ("#4c72b0", "o: satisfies Hamming comparison"),
            ("#dd8452", "x: soft counterexample"),
            ("#c44e52", "X: hard counterexample"),
            ("#f2f2f2", "~: outside k < n"),
            ("#d9d9d9", "?: rejected by guard"),
            ("#ffffff", ".: not yet run"),
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
