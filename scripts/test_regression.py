#!/usr/bin/env python3
"""Run validator outputs against the checked-in golden oracle."""

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORACLE = ROOT / "tests" / "res/oracle_baseline.json"
BINARY = Path(os.environ.get("VALIDATOR_BIN", ROOT / "bin" / "validate_extra_cut"))


def main() -> int:
    """Run every recorded validator case and compare its JSON output."""
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--oracle", type=Path, default=ORACLE)
    parser.add_argument(
        "--max-vertices",
        type=int,
        help="only run oracle cases no larger than this graph size",
    )
    args = parser.parse_args()

    oracle = args.oracle if args.oracle.is_absolute() else ROOT / args.oracle
    cases = json.loads(oracle.read_text())["cases"]
    if args.max_vertices is not None:
        cases = [case for case in cases if case["valid_vertices"] <= args.max_vertices]
    if not cases:
        print("No oracle cases match the selected size limit.", file=sys.stderr)
        return 2

    failures = 0
    for case in cases:
        n, k = case["n"], case["k"]
        print(
            f"A({n},{k}) R={case['R']} g={case['g']} ...",
            end=" ",
            flush=True,
        )
        try:
            result = subprocess.run(
                [str(BINARY), str(n), str(k), "--json"],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            output = json.loads(result.stdout)
            deep_check = output.get("deep_check", "completed")
            actual = {
                "n": output["graph"]["n"],
                "k": output["graph"]["k"],
                "m": output["graph"]["m"],
                "valid_vertices": output["graph"]["valid_vertices"],
                "g": output["cut_properties"]["g"],
                "R": output["cut_properties"]["R"],
                "boundary": output["cut_properties"]["actual_boundary"],
                "components": (
                    None
                    if deep_check == "skipped"
                    else sorted(output["component_sizes"])
                ),
                "hamming_boundary": output["hamming_comparison"]["hamming_boundary"],
                "embedding_gate": output["hamming_comparison"]["embedding_gate"],
                "classification": output["hamming_comparison"]["classification"],
            }
        except (
            OSError,
            subprocess.CalledProcessError,
            json.JSONDecodeError,
            KeyError,
        ) as error:
            print(f"FAIL ({error})")
            failures += 1
            continue

        mismatches = [
            f"{key}: expected {case[key]!r}, got {value!r}"
            for key, value in actual.items()
            if value != case[key]
        ]
        if mismatches:
            print("FAIL")
            for mismatch in mismatches:
                print(f"  - {mismatch}")
            failures += 1
        else:
            gate = "open" if actual["embedding_gate"] else "closed"
            print(
                f"PASS | boundary={actual['boundary']} gate={gate} | "
                f"{actual['classification']}"
            )

    if failures:
        print(f"\nRegression failed: {failures} case(s) mismatched.")
        return 1
    print("\nAll validator oracle cases passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
