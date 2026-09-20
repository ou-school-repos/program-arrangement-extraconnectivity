#!/usr/bin/env python3
"""Regression checks for the small exact connected-profile oracle."""

import os
import re
import subprocess
import sys


def profile(binary, n, k, max_size):
    """Return the state count and connected profile for each size."""
    output = subprocess.check_output(
        [binary, str(n), str(k), str(max_size), "200000"], text=True
    )
    result = {}
    for line in output.splitlines():
        match = re.match(r"s=(\d+) states=(\d+) Phi_conn=(\d+)", line)
        if match:
            result[int(match.group(1))] = (
                int(match.group(2)),
                int(match.group(3)),
            )
    return result


def main():
    """Check known exact connected-profile values against the executable."""
    binary = os.environ.get("PROFILE_BIN", "./bin/exact_profile_naive")
    a53 = profile(binary, 5, 3, 7)
    assert a53[7] == (70182, 17), a53.get(7)

    a65 = profile(binary, 6, 5, 6)
    assert a65[6] == (10576, 18), a65.get(6)

    print("exact-profile regression: PASS")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, subprocess.CalledProcessError) as error:
        print(f"exact-profile regression: FAIL: {error}", file=sys.stderr)
        raise
