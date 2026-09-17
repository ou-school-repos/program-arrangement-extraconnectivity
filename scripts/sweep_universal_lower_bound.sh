#!/bin/bash
# Exhaustive UniversalLowerBound sweep.
#
# Cross-check the transparent Python oracle where the row is small enough;
# otherwise run the faster C++ verifier alone, subject to an explicit cap.

set -uo pipefail

PY=scripts/check_universal_lower_bound.py
CC=bin/universal_lower_bound
PY_MAX_SUBSETS=${PY_MAX_SUBSETS:-1000000}
MAX_SUBSETS=${MAX_SUBSETS:-250000000}

cross_checked=0
cxx_checked=0
skipped=0
failed=0

subset_count() {
	python3 - "$1" "$2" "$3" <<'PY'
from math import comb, perm
import sys
n, k, r = map(int, sys.argv[1:])
print(comb(perm(n, k), r))
PY
}

run() {
	local n=$1 k=$2 r=$3 subsets py_out cc_out
	subsets=$(subset_count "$n" "$k" "$r")
	printf '=== A(%d,%d) R=%d: %s subsets ===\n' "$n" "$k" "$r" "$subsets"

	if ((subsets > MAX_SUBSETS)); then
		printf '  SKIP: exceeds MAX_SUBSETS=%s; rerun with a deliberate larger limit.\n\n' \
			"$MAX_SUBSETS"
		((++skipped))
		return 0
	fi

	if ! cc_out=$("$CC" "$n" "$k" "$r" --max-subsets "$MAX_SUBSETS" 2>&1); then
		printf '  C++ FAILED: %s\n' "$cc_out"
		((++failed))
		return 1
	fi
	printf '  CC: %s\n' "$cc_out"
	((++cxx_checked))

	if ((subsets > PY_MAX_SUBSETS)); then
		printf '  Python skipped: exceeds PY_MAX_SUBSETS=%s.\n\n' "$PY_MAX_SUBSETS"
		return 0
	fi
	if ! py_out=$(python3 "$PY" "$n" "$k" "$r" 2>&1); then
		printf '  Python FAILED: %s\n' "$py_out"
		((++failed))
		return 1
	fi
	printf '  PY: %s\n' "$py_out"
	if [[ "$py_out" != "$cc_out" ]]; then
		printf '  MISMATCH\n\n'
		((++failed))
		return 1
	fi
	printf '  Cross-check: OK\n\n'
	((++cross_checked))
}

# Previously untested n-k=3 cases.
run 6 3 3 || exit 1
run 6 3 4 || exit 1
run 6 3 5 || exit 1

# A(7,2) extension (n-k=5).
run 7 2 3 || exit 1
run 7 2 4 || exit 1
run 7 2 5 || exit 1
run 7 2 6 || exit 1

# A(5,3) extension (n-k=2).
run 5 3 3 || exit 1
run 5 3 4 || exit 1
run 5 3 5 || exit 1

# A(5,4), n-k=1 regime.
run 5 4 3 || exit 1
run 5 4 4 || exit 1

# A(6,4), n-k=2, small R.
run 6 4 2 || exit 1
run 6 4 3 || exit 1

# A(8,2), n-k=6 extension.
run 8 2 3 || exit 1
run 8 2 4 || exit 1

# A(4,3), n-k=1, full sweep.
run 4 3 3 || exit 1
run 4 3 4 || exit 1
run 4 3 5 || exit 1
run 4 3 6 || exit 1

# A(6,5), n-k=1, R=2,3.
run 6 5 2 || exit 1
run 6 5 3 || exit 1

# A(3,2), n-k=1, R=2,3.
run 3 2 2 || exit 1
run 3 2 3 || exit 1

printf '================================\n'
printf 'C++ rows: %d  Cross-checked rows: %d  Skipped: %d  Failed: %d\n' \
	"$cxx_checked" "$cross_checked" "$skipped" "$failed"
((failed == 0)) && echo 'ALL COMPLETED ROWS CLEAR'
