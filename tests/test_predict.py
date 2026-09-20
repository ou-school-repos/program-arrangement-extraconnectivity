#!/usr/bin/env python3
"""Check predicted Hamming boundaries against direct construction counts."""

import argparse
import subprocess
import sys


def run_output(binary: str, *args: str) -> str:
    """Run a binary and return stdout, raising with stderr on failure."""
    result = subprocess.run(
        [binary, *args],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"{binary} {' '.join(args)} exited with {result.returncode}: "
            f"{result.stderr.strip()}"
        )
    return result.stdout


def main() -> int:
    """Verify the predictor's formula and witness for each requested radius."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-r", type=int, default=8)
    args = parser.parse_args()

    if args.max_r < 2:
        parser.error("--max-r must be at least 2")

    try:
        output = run_output("./bin/predict", "--verify-range", "2", str(args.max_r))
    except (OSError, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1

    rows = output.splitlines()
    if not rows or rows[0] != "R,nk1,constant,coeff,formula_at_2R":
        print("ERROR: predictor returned an invalid CSV header", file=sys.stderr)
        return 1
    radii = [int(row.split(",", 1)[0]) for row in rows[1:] if row]
    expected_radii = list(range(2, args.max_r + 1))
    if radii != expected_radii:
        print(f"ERROR: expected radii {expected_radii}, got {radii}", file=sys.stderr)
        return 1

    for r in radii:
        print(f"R={r}: Hamming witness matches direct boundary count.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
