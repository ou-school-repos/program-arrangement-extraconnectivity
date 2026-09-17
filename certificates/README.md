# Computer-assisted certificate tools

These programs support the finite certificate reported in the paper. They are
ordinary C++/shell programs, not Lean proofs.

`fdp_certificate.cpp` computes a sound relaxed upper bound `F >= max Q` from the
slice identity, line-slot capacity, local cap, and defect averaging.
`max_q_oracle.cpp` exhaustively computes the exact maximum on small graphs.
`soundness_check.sh` compares the two on the checked cells. `audit_orbits.cpp`
provides Burnside orbit-count regressions, and `opt_orbits.cpp` checks finite
equality-orbit evidence. `defect_spectrum.cpp` remains in `scripts/unstable/`
because it is exploratory and has no certificate role.

Example build from the repository root:

```sh
mkdir -p /tmp/arrangement-certificates
g++ -O2 -std=c++17 certificates/fdp_certificate.cpp \
  -o /tmp/arrangement-certificates/fdp_certificate
g++ -O2 -std=c++17 certificates/max_q_oracle.cpp \
  -o /tmp/arrangement-certificates/max_q_oracle
```

The finite outputs are evidence only. They do not prove the open universal
restricted boundary inequality, and they are not checked by Lean.
