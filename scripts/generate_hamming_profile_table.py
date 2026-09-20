#!/usr/bin/env python3
"""Generate the per-cell table of currently computed exact profiles.

The profile values and Hamming-sharp ranges are transcribed from
docs/small-exact-profile-results.md. This is a finite computational record,
not a claim for all arrangement graphs or all R < 11.
"""

from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

# (n, k, exact profile values indexed from R=1, Hamming-sharp R values)
CASES = (
    (6, 3, (9, 14, 18, 20, 23, 24, 25), set(range(1, 8))),
    (5, 3, (6, 9, 11, 12, 14, 15, 17), set(range(1, 5))),
    (5, 4, (4, 6, 8, 10, 11, 12, 14, 16), set(range(1, 3))),
    (6, 4, (8, 13, 17, 20, 24, 27), set(range(1, 5))),
)

FIELDS = (
    "n",
    "k",
    "m",
    "R",
    "exact_profile",
    "cube_dimension",
    "embedding_gate",
    "hamming_sharp",
    "scope",
)


def rows() -> list[dict[str, object]]:
    """Return one row for each currently documented exact-profile cell."""
    result = []
    for n, k, profile, sharp_radii in CASES:
        m = n - k
        for radius, exact_profile in enumerate(profile, start=1):
            dimension = (radius - 1).bit_length()
            gate = dimension <= k and dimension <= m
            result.append(
                {
                    "n": n,
                    "k": k,
                    "m": m,
                    "R": radius,
                    "exact_profile": exact_profile,
                    "cube_dimension": dimension,
                    "embedding_gate": gate,
                    "hamming_sharp": radius in sharp_radii,
                    "scope": "computed exact profile",
                }
            )
    return result


def write_csv(data: list[dict[str, object]]) -> None:
    """Write the cell data as a CSV file under docs/."""
    with (DOCS / "hamming_profile_cells.csv").open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(data)


def write_markdown(data: list[dict[str, object]]) -> None:
    """Write the cell data as a readable Markdown table under docs/."""
    lines = [
        "# Computed Hamming profile cells",
        "",
        "Per-cell expansion of the exact profile results in "
        "[small-exact-profile-results.md](small-exact-profile-results.md). "
        "The data covers only the graph parameters with computed exact "
        "profiles; it does not assert Hamming optimality for every graph "
        "with `R < 11`.",
        "",
        "`embedding_gate` says the Boolean-cube witness exists. "
        "`hamming_sharp` says the computed exact profile equals that witness "
        "boundary. A closed gate means the Hamming expression is not an "
        "available witness.",
        "",
        "| n | k | m | R | exact profile | d | gate | Hamming sharp |",
        "|--:|--:|--:|--:|--------------:|--:|:----:|:-------------:|",
    ]
    for row in data:
        lines.append(
            f"| {row['n']} | {row['k']} | {row['m']} | {row['R']} | "
            f"{row['exact_profile']} | {row['cube_dimension']} | "
            f"{'open' if row['embedding_gate'] else 'closed'} | "
            f"{'yes' if row['hamming_sharp'] else 'no'} |"
        )
    lines.append("")
    (DOCS / "hamming_profile_cells.md").write_text("\n".join(lines))


def main() -> None:
    """Generate both table formats from the embedded exact-profile data."""
    data = rows()
    write_csv(data)
    write_markdown(data)
    print(
        f"Wrote {len(data)} exact-profile cells to "
        "docs/hamming_profile_cells.csv and .md"
    )


if __name__ == "__main__":
    main()
