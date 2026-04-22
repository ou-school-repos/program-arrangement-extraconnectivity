#!/usr/bin/env python3


def to_graph6(N, edges):
    n_char = chr(N + 63)

    bits = ""
    for i in range(1, N):
        for j in range(i):
            if (i, j) in edges or (j, i) in edges:
                bits += "1"
            else:
                bits += "0"

    # Pad to multiple of 6
    while len(bits) % 6 != 0:
        bits += "0"

    chars = [chr(int(bits[k : k + 6], 2) + 63) for k in range(0, len(bits), 6)]
    return n_char + "".join(chars)


edges9 = [
    (0, 1),
    (0, 2),
    (0, 3),
    (0, 4),
    (1, 5),
    (1, 6),
    (2, 5),
    (2, 7),
    (3, 6),
    (3, 7),
    (5, 8),
    (6, 8),
    (7, 8),
]

edges10 = [
    (0, 1),
    (0, 2),
    (0, 3),
    (0, 4),
    (1, 5),
    (1, 6),
    (1, 7),
    (2, 5),
    (2, 8),
    (3, 6),
    (3, 8),
    (4, 7),
    (5, 9),
    (6, 9),
    (8, 9),
]

print("R=9:  " + to_graph6(9, edges9))
print("R=10: " + to_graph6(10, edges10))
