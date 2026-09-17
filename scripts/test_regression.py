#!/usr/bin/env python3
"""Run validator outputs against the checked-in golden oracle."""

import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORACLE = ROOT / "tests" / "oracle_baseline.json"
BINARY = Path(os.environ.get("VALIDATOR_BIN", ROOT / "bin" / "validate_extra_cut"))


def main() -> int:
    """Run every recorded validator case and compare its JSON output."""
    cases = json.loads(ORACLE.read_text())["cases"]
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
            actual = {
                "n": output["graph"]["n"],
                "k": output["graph"]["k"],
                "m": output["graph"]["m"],
                "valid_vertices": output["graph"]["valid_vertices"],
                "g": output["cut_properties"]["g"],
                "R": output["cut_properties"]["R"],
                "boundary": output["cut_properties"]["actual_boundary"],
                "components": sorted(output["component_sizes"]),
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
