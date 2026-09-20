"""Check budget-filtered defect-collision hulls from catalogue signatures.

Usage: ``hull_check.py R signature-file max-k max-m [file max-k max-m ...]``.
For each feasible budget cell, report upper-hull vertices and their exact
support windows for the score ``(m + 1) * D + X``.
"""

import sys
from fractions import Fraction
from pathlib import Path

Signature = tuple[int, int, int, int]
Point = tuple[int, int]


def load_signatures(filename: str) -> list[Signature]:
    """Read D, X, active-position count, and symbol count from a catalogue."""
    signatures = []
    with Path(filename).open(encoding="utf-8") as input_file:
        for line in input_file:
            if line[:1].isdigit():
                defect, collisions, positions, symbols = map(int, line.split()[:4])
                signatures.append((defect, collisions, positions, symbols - positions))
    return signatures


def upper_hull(points: list[Point]) -> list[tuple[Point, Fraction, Fraction | None]]:
    """Return hull vertices with the adjacent exact slope support limits."""
    best_by_defect: dict[int, int] = {}
    for defect, collisions in points:
        best_by_defect[defect] = max(best_by_defect.get(defect, -1), collisions)

    nondominated = []
    for point in reversed(sorted(best_by_defect.items())):
        if not nondominated or point[1] > nondominated[-1][1]:
            nondominated.append(point)
    nondominated.reverse()

    vertices: list[Point] = []
    for point in nondominated:
        while len(vertices) >= 2:
            first = vertices[-2]
            second = vertices[-1]
            first_slope = Fraction(first[1] - second[1], second[0] - first[0])
            next_slope = Fraction(second[1] - point[1], point[0] - second[0])
            if first_slope >= next_slope:
                vertices.pop()
            else:
                break
        vertices.append(point)

    slopes = [
        Fraction(left[1] - right[1], right[0] - left[0])
        for left, right in zip(vertices, vertices[1:])
    ]
    return [
        (
            point,
            Fraction(0) if index == 0 else slopes[index - 1],
            None if index == len(vertices) - 1 else slopes[index],
        )
        for index, point in enumerate(vertices)
    ]


def boundary(radius: int, positions: int, slack: int, point: Point) -> int:
    """Evaluate the boundary formula for one (D, X) signature."""
    defect, collisions = point
    return (radius * positions - defect) * slack - defect - collisions


def format_limit(value: Fraction | None) -> str:
    """Format a support-window endpoint, including the unbounded endpoint."""
    return "inf" if value is None else str(value)


def check_host(
    radius: int,
    filename: str,
    max_positions: int,
    max_slack: int,
) -> int:
    """Print hulls over all feasible budget cells covered by one host."""
    signatures = load_signatures(filename)
    interior_hits = 0
    for positions in range(1, max_positions + 1):
        for slack in range(1, max_slack + 1):
            feasible = [
                (defect, collisions)
                for defect, collisions, active, extra in signatures
                if active <= positions and extra <= slack
            ]
            if not feasible:
                continue

            hull = upper_hull(feasible)
            score_parameter = slack + 1
            best_boundary = min(
                boundary(radius, positions, slack, point) for point in feasible
            )
            winners = sorted(
                {
                    point
                    for point in feasible
                    if boundary(radius, positions, slack, point) == best_boundary
                }
            )
            a_extreme = hull[-1][0]
            b_extreme = hull[0][0]
            strict_interior = [
                point
                for point, lower, upper in hull[1:-1]
                if upper is not None and lower < score_parameter < upper
            ]
            interior_hits += len(strict_interior)
            windows = [
                (point, format_limit(lower), format_limit(upper))
                for point, lower, upper in hull
            ]
            suffix = (
                f"  <-- INTERIOR WINNER {strict_interior}" if strict_interior else ""
            )
            print(
                f"k={positions} m={slack} t={score_parameter}: "
                f"Phi_conn={best_boundary} winners={winners} "
                f"A*={a_extreme} B*={b_extreme} hull-windows={windows}{suffix}"
            )
    print(
        f"R={radius} {filename}: interior hull vertices strictly winning "
        f"in some cell: {interior_hits}"
    )
    return interior_hits


def main() -> None:
    """Run the hull audit for one or more finite catalogue hosts."""
    arguments = sys.argv[1:]
    if len(arguments) < 4 or (len(arguments) - 1) % 3 != 0:
        raise SystemExit(
            "Usage: hull_check.py R signature-file max-k max-m "
            "[signature-file max-k max-m ...]"
        )
    radius = int(arguments[0])
    total_hits = 0
    for index in range(1, len(arguments), 3):
        total_hits += check_host(
            radius,
            arguments[index],
            int(arguments[index + 1]),
            int(arguments[index + 2]),
        )
    print(f"R={radius}: total interior strict winners: {total_hits}")


if __name__ == "__main__":
    main()
