#!/usr/bin/env python3
"""Trace the feasible graph-connected signature envelope for fixed R and k.

Read `pattern_catalogue` output from a file or stdin. For each integer slack
`m`, report the minimum boundary and all signatures attaining it. Optionally
plot the feasible affine boundary lines and their integer lower envelope.

Example:
    ./bin/pattern_catalogue 7 --host 8 3 > /tmp/patterns.txt
    python3 scripts/trace_pattern_envelope.py 7 3 --max-m 5 \
        --catalogue /tmp/patterns.txt --plot /tmp/envelope.png

This traces the graph-connected catalogue only. It is not, by itself, the
unrestricted vertex-isoperimetric profile.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO


@dataclass(frozen=True, order=True)
class Signature:
    """A pattern signature and its spare-symbol budget."""

    defect: int
    collisions: int
    active_coordinates: int
    active_symbols: int

    @property
    def extra_symbols(self) -> int:
        """Return the number of symbols beyond the base-coordinate symbols."""
        return self.active_symbols - self.active_coordinates

    def boundary(self, volume: int, dimension: int, slack: int) -> int:
        """Evaluate the exact boundary line at the requested cell."""
        return (
            (volume * dimension - self.defect) * slack - self.defect - self.collisions
        )


HEADER = re.compile(
    r"^R=(?P<r>\d+) host=A\((?P<n>\d+),(?P<k>\d+)\) "
    r"\[complete for cells k<=K, n-k<=N-K\]"
)
SIGNATURE = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+")


def read_catalogue(stream: TextIO) -> tuple[int, int, int, set[Signature]]:
    """Parse the catalogue header and signature rows."""
    volume = host_n = host_k = None
    signatures: set[Signature] = set()
    for line in stream:
        if match := HEADER.match(line):
            volume = int(match.group("r"))
            host_n = int(match.group("n"))
            host_k = int(match.group("k"))
            continue
        if match := SIGNATURE.match(line):
            signatures.add(Signature(*(int(group) for group in match.groups())))
    if volume is None or host_n is None or host_k is None:
        raise ValueError("input has no pattern_catalogue host header")
    if not signatures:
        raise ValueError("input has no signature rows")
    return volume, host_n, host_k, signatures


def feasible_signatures(
    signatures: set[Signature], dimension: int, slack: int
) -> list[Signature]:
    """Keep signatures satisfying the coordinate and symbol budgets."""
    return sorted(
        signature
        for signature in signatures
        if signature.active_coordinates <= dimension
        and signature.extra_symbols <= slack
    )


def plot_envelope(
    signatures: set[Signature],
    volume: int,
    dimension: int,
    max_slack: int,
    destination: Path,
) -> None:
    """Save a plot of feasible signature lines and the integer envelope."""
    try:
        import matplotlib.pyplot as plt  # pylint: disable=import-outside-toplevel
    except ImportError as error:
        raise SystemExit("--plot requires matplotlib") from error

    candidates = [
        signature
        for signature in signatures
        if signature.active_coordinates <= dimension
        and signature.extra_symbols <= max_slack
    ]
    values: list[tuple[int, int]] = []
    for slack in range(1, max_slack + 1):
        feasible = feasible_signatures(signatures, dimension, slack)
        if feasible:
            values.append(
                (
                    slack,
                    min(s.boundary(volume, dimension, slack) for s in feasible),
                )
            )

    figure, axis = plt.subplots(figsize=(10, 6))
    for signature in candidates:
        start = max(1, signature.extra_symbols)
        if start > max_slack:
            continue
        xs = list(range(start, max_slack + 1))
        ys = [signature.boundary(volume, dimension, slack) for slack in xs]
        axis.plot(xs, ys, alpha=0.25, linewidth=0.8)
    if values:
        axis.plot(
            [point[0] for point in values],
            [point[1] for point in values],
            color="black",
            marker="o",
            linewidth=2.5,
            label="integer lower envelope",
        )
        axis.legend()
    axis.set(
        xlabel="spare symbols m = n - k",
        ylabel="external boundary size",
        title=f"Graph-connected signature envelope: R={volume}, k={dimension}",
    )
    axis.grid(True, alpha=0.25)
    figure.tight_layout()
    figure.savefig(destination, dpi=180)
    plt.close(figure)


def main() -> int:
    """Parse arguments, print the envelope, and optionally draw it."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("R", type=int, help="pattern volume")
    parser.add_argument("k", type=int, help="fixed active dimension")
    parser.add_argument("--max-m", type=int, default=30, help="largest slack to scan")
    parser.add_argument(
        "--catalogue",
        type=Path,
        help="pattern_catalogue output file (defaults to standard input)",
    )
    parser.add_argument("--plot", type=Path, help="optional output image path")
    args = parser.parse_args()

    if args.R < 1 or args.k < 1 or args.max_m < 1:
        parser.error("require R >= 1, k >= 1, and --max-m >= 1")

    try:
        if args.catalogue:
            with args.catalogue.open(encoding="utf-8") as stream:
                volume, host_n, host_k, signatures = read_catalogue(stream)
        else:
            volume, host_n, host_k, signatures = read_catalogue(sys.stdin)
    except (OSError, ValueError) as error:
        parser.error(str(error))

    if volume != args.R:
        parser.error(f"catalogue is for R={volume}, but requested R={args.R}")
    if args.k > host_k or args.max_m > host_n - host_k:
        parser.error(
            "requested cells exceed this catalogue host's completeness range: "
            f"k <= {host_k}, m <= {host_n - host_k}"
        )

    print(
        f"R={volume}, k={args.k}: graph-connected profile "
        f"(catalogue host A({host_n},{host_k}))"
    )
    print("m  n  boundary  winning signatures (D,X,p,s_a)")
    previous: tuple[Signature, ...] | None = None
    for slack in range(1, args.max_m + 1):
        feasible = feasible_signatures(signatures, args.k, slack)
        if not feasible:
            print(f"{slack:2} {args.k + slack:2}  no feasible signature")
            continue
        best = min(s.boundary(volume, args.k, slack) for s in feasible)
        winners = tuple(
            s for s in feasible if s.boundary(volume, args.k, slack) == best
        )
        label = " phase change" if previous is not None and winners != previous else ""
        formatted = ", ".join(
            f"({s.defect},{s.collisions},{s.active_coordinates},{s.active_symbols})"
            for s in winners
        )
        print(f"{slack:2} {args.k + slack:2} {best:9}  {formatted}{label}")
        previous = winners

    if args.plot:
        plot_envelope(signatures, volume, args.k, args.max_m, args.plot)
        print(f"plot written to {args.plot}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
