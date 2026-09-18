#!/usr/bin/env python3
"""Compare the predictor against the trusted arrangement search."""

import argparse
import subprocess
import sys


def run_output(binary: str, r: int) -> str:
    """Run a validator binary for one radius and return its stdout."""
    result = subprocess.run(
        [binary, str(r)],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"{binary} R={r} exited with {result.returncode}: {result.stderr.strip()}"
        )
    return result.stdout


def comparable_value(output: str, *, search: bool) -> str:
    """Extract the comparable formula prefix from tool output."""
    lines = output.splitlines()
    if search:
        lines = [line for line in lines if "EX:" in line]
        if not lines:
            raise ValueError("search output contains no EX: line")
        output = lines[-1]
    return output.split(",", 1)[0].replace(" ", "").strip()


def main() -> int:
    """Compare predictor output against the arrangement search."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-r", type=int, default=8)
    args = parser.parse_args()

    for r in range(2, args.max_r + 1):
        try:
            expected = comparable_value(run_output("./bin/arrangement", r), search=True)
            actual = comparable_value(run_output("./bin/predict", r), search=False)
        except (OSError, RuntimeError, ValueError) as error:
            print(f"R={r}: ERROR: {error}", file=sys.stderr)
            return 1

        if actual != expected:
            print(f"R={r}: mismatch")
            print(f"  search:  {expected}")
            print(f"  predict: {actual}")
            return 1
        print(f"R={r}: prediction matches search.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
