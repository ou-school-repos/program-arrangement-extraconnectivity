# Computed Hamming profile cells

Per-cell expansion of the exact profile results in
[small-exact-profile-results.md](small-exact-profile-results.md). The data
covers only the graph parameters with computed exact profiles; it does not
assert Hamming optimality for every graph with `R < 11`.

`embedding_gate` says the Boolean-cube witness exists. `hamming_sharp` says the
computed exact profile equals that witness boundary. A closed gate means the
Hamming expression is not an available witness.

|   n |   k |   m |   R | exact profile |   d |  gate  | Hamming sharp |
| --: | --: | --: | --: | ------------: | --: | :----: | :-----------: |
|   6 |   3 |   3 |   1 |             9 |   0 |  open  |      yes      |
|   6 |   3 |   3 |   2 |            14 |   1 |  open  |      yes      |
|   6 |   3 |   3 |   3 |            18 |   2 |  open  |      yes      |
|   6 |   3 |   3 |   4 |            20 |   2 |  open  |      yes      |
|   6 |   3 |   3 |   5 |            23 |   3 |  open  |      yes      |
|   6 |   3 |   3 |   6 |            24 |   3 |  open  |      yes      |
|   6 |   3 |   3 |   7 |            25 |   3 |  open  |      yes      |
|   5 |   3 |   2 |   1 |             6 |   0 |  open  |      yes      |
|   5 |   3 |   2 |   2 |             9 |   1 |  open  |      yes      |
|   5 |   3 |   2 |   3 |            11 |   2 |  open  |      yes      |
|   5 |   3 |   2 |   4 |            12 |   2 |  open  |      yes      |
|   5 |   3 |   2 |   5 |            14 |   3 | closed |      no       |
|   5 |   3 |   2 |   6 |            15 |   3 | closed |      no       |
|   5 |   3 |   2 |   7 |            17 |   3 | closed |      no       |
|   5 |   4 |   1 |   1 |             4 |   0 |  open  |      yes      |
|   5 |   4 |   1 |   2 |             6 |   1 |  open  |      yes      |
|   5 |   4 |   1 |   3 |             8 |   2 | closed |      no       |
|   5 |   4 |   1 |   4 |            10 |   2 | closed |      no       |
|   5 |   4 |   1 |   5 |            11 |   3 | closed |      no       |
|   5 |   4 |   1 |   6 |            12 |   3 | closed |      no       |
|   5 |   4 |   1 |   7 |            14 |   3 | closed |      no       |
|   5 |   4 |   1 |   8 |            16 |   3 | closed |      no       |
|   6 |   4 |   2 |   1 |             8 |   0 |  open  |      yes      |
|   6 |   4 |   2 |   2 |            13 |   1 |  open  |      yes      |
|   6 |   4 |   2 |   3 |            17 |   2 |  open  |      yes      |
|   6 |   4 |   2 |   4 |            20 |   2 |  open  |      yes      |
|   6 |   4 |   2 |   5 |            24 |   3 | closed |      no       |
|   6 |   4 |   2 |   6 |            27 |   3 | closed |      no       |
