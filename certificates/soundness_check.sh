#!/usr/bin/env bash
# Soundness regression: F_{m,k}(R) from fdp_certificate must be >= exhaustive max Q.
set -euo pipefail
fail=0
check() { # n k maxR
	local n=$1 k=$2 maxr=$3 m=$(($1 - $2))
	local table
	table=$(./fdp_certificate "$m" "$k" "$maxr" --detail | grep "^  k=$k ")
	for R in $(seq 1 "$maxr"); do
		local line F q
		line=$(./max_q_oracle "$n" "$k" "$R" 2>/dev/null) || continue
		q=$(sed -E 's/.*maxQ=(-?[0-9]+).*/\1/' <<<"$line")
		F=$(grep -E " R=$R F=" <<<"$table" | sed -E 's/.*F=(-?[0-9]+).*/\1/' || true)
		if [[ -z "$F" ]]; then
			echo "UNSOUND(infeasible) A($n,$k) R=$R true=$q"
			fail=1
			continue
		fi
		if ((F < q)); then
			echo "UNSOUND A($n,$k) R=$R F=$F true=$q"
			fail=1
		fi
		echo "A($n,$k) R=$R true=$q F=$F G=$(sed -E 's/.*G=(-?[0-9]+).*/\1/' <<<"$line")"
	done
}
check 3 2 6
check 4 2 12
check 5 2 9
check 6 2 7
check 7 2 5
check 4 3 8
check 5 3 7
check 6 3 5
check 7 3 4
check 5 4 5
check 6 4 4
check 7 4 3
check 6 5 4
check 7 5 3
exit $fail
