#!/usr/bin/env python3
"""Search arrangement-graph subsets by connected boundary-preserving swaps.

The default instance is A(10, 5), R=26, where the embedded binary Hamming
ball has external boundary 298.  A move removes one current vertex and adds a
non-member from the current external boundary.  Boundary counts are updated
incrementally, while every accepted candidate is required to remain connected.

This is a heuristic counterexample finder, not a proof of optimality.  Any
reported incumbent is checked once more by a from-scratch boundary scan.
"""

from __future__ import annotations

import argparse
import math
import random
import time
from itertools import permutations


def all_vertices(n: int, k: int) -> tuple[tuple[int, ...], ...]:
    """Enumerate the vertices of the arrangement graph."""
    return tuple(permutations(range(n), k))


def neighbors(vertex: tuple[int, ...], n: int) -> list[tuple[int, ...]]:
    """Return all one-coordinate substitution neighbors of a vertex."""
    used = set(vertex)
    result: list[tuple[int, ...]] = []
    for p in range(len(vertex)):
        for symbol in range(n):
            if symbol not in used:
                result.append(vertex[:p] + (symbol,) + vertex[p + 1 :])
    return result


def hamming_ball(k: int, size: int) -> list[tuple[int, ...]]:
    """The usual binary slice, using fresh symbols k..k+d-1."""
    dimensions = (size - 1).bit_length()
    base = tuple(range(k))
    result = []
    for index in range(size):
        vertex = list(base)
        for bit in range(dimensions):
            if index & (1 << bit):
                vertex[bit] = k + bit
        result.append(tuple(vertex))
    return result


def connected(
    members: set[int], adjacency: tuple[tuple[int, ...], ...], start: int
) -> bool:
    """Test connectivity of the induced subgraph on ``members``."""
    seen = {start}
    stack = [start]
    while stack:
        vertex = stack.pop()
        for neighbor in adjacency[vertex]:
            if neighbor in members and neighbor not in seen:
                seen.add(neighbor)
                stack.append(neighbor)
    return len(seen) == len(members)


def exact_boundary(
    members: set[int], adjacency: tuple[tuple[int, ...], ...]
) -> set[int]:
    """Compute the external vertex boundary from scratch."""
    boundary: set[int] = set()
    for vertex in members:
        boundary.update(
            neighbor for neighbor in adjacency[vertex] if neighbor not in members
        )
    return boundary


class State:  # pylint: disable=too-few-public-methods
    """Mutable search state with incrementally maintained boundary counts."""

    def __init__(self, initial: list[int], adjacency: tuple[tuple[int, ...], ...]):
        self.members = set(initial)
        self.adjacency = adjacency
        self.counts = [0] * len(adjacency)
        for vertex in initial:
            for neighbor in adjacency[vertex]:
                self.counts[neighbor] += 1
        self.boundary = {
            v for v, count in enumerate(self.counts) if count and v not in self.members
        }
        self.score = len(self.boundary)

    def swap(self, outgoing: int, incoming: int) -> tuple[int, set[int]]:
        """Apply a swap and return (new boundary size, changed vertices)."""
        changed = set()
        self.members.remove(outgoing)
        for neighbor in self.adjacency[outgoing]:
            before = self.counts[neighbor]
            self.counts[neighbor] -= 1
            if before != self.counts[neighbor]:
                changed.add(neighbor)
        self.members.add(incoming)
        for neighbor in self.adjacency[incoming]:
            before = self.counts[neighbor]
            self.counts[neighbor] += 1
            if before != self.counts[neighbor]:
                changed.add(neighbor)
        changed.update((outgoing, incoming))
        for vertex in changed:
            if vertex in self.members or self.counts[vertex] == 0:
                self.boundary.discard(vertex)
            else:
                self.boundary.add(vertex)
        self.score = len(self.boundary)
        return self.score, changed


def run(args: argparse.Namespace) -> int:
    """Run the requested annealing search and print its best incumbent."""
    if args.n < 1 or args.k < 1 or args.k > args.n:
        raise SystemExit(f"require 1 <= k <= n, got k={args.k}, n={args.n}")
    if args.size < 1:
        raise SystemExit(f"require size >= 1, got size={args.size}")
    if args.steps < 0 or args.restarts < 1:
        raise SystemExit("require steps >= 0 and restarts >= 1")
    if (
        not math.isfinite(args.temperature)
        or args.temperature <= 0
        or not math.isfinite(args.cooling)
        or not 0 < args.cooling <= 1
    ):
        raise SystemExit("require finite temperature > 0 and 0 < cooling <= 1")
    max_size = 2 ** min(args.k, args.n - args.k)
    if args.size > max_size:
        raise SystemExit(
            f"require size <= 2**min(k, n-k) = {max_size}, got size={args.size}"
        )

    vertex_count = 1
    for offset in range(args.k):
        factor = args.n - offset
        if vertex_count > 100_000 // factor:
            raise SystemExit("arrangement graph exceeds 100000 vertices")
        vertex_count *= factor
    degree = args.k * (args.n - args.k)
    if degree and vertex_count > 10_000_000 // degree:
        raise SystemExit("arrangement graph exceeds adjacency resource limit")

    vertices = all_vertices(args.n, args.k)
    index = {vertex: i for i, vertex in enumerate(vertices)}
    adjacency = tuple(
        tuple(index[v] for v in neighbors(vertex, args.n)) for vertex in vertices
    )
    seed = [index[v] for v in hamming_ball(args.k, args.size)]
    initial = State(seed, adjacency)
    verified = exact_boundary(initial.members, adjacency)
    if len(verified) != initial.score:
        raise AssertionError("incremental initialization disagrees with exact boundary")
    best_score = initial.score
    best_members = set(initial.members)
    rng = random.Random(args.seed)
    started = time.monotonic()
    accepted = 0
    attempted = 0

    print(f"A({args.n},{args.k}) R={args.size}: initial boundary={best_score}")
    for restart in range(args.restarts):
        state = (
            State(seed, adjacency)
            if restart == 0
            else State(list(best_members), adjacency)
        )
        temperature = args.temperature
        for step in range(args.steps):
            if not state.boundary:
                break
            attempted += 1
            outgoing = rng.choice(tuple(state.members))
            incoming = rng.choice(tuple(state.boundary))
            old_score = state.score
            state.swap(outgoing, incoming)
            delta = state.score - old_score
            is_connected = connected(state.members, adjacency, incoming)
            accept = is_connected and (
                delta <= 0 or rng.random() < math.exp(-delta / max(temperature, 1e-9))
            )
            if accept:
                accepted += 1
                if state.score < best_score:
                    best_score = state.score
                    best_members = set(state.members)
                    checked = exact_boundary(best_members, adjacency)
                    assert len(checked) == best_score
                    print(
                        f"hit boundary={best_score} restart={restart} "
                        f"step={step} elapsed={time.monotonic() - started:.1f}s"
                    )
                    if best_score < args.target:
                        break
            else:
                state.swap(outgoing=incoming, incoming=outgoing)
            temperature *= args.cooling
            if temperature < 0.05:
                temperature = 0.5 * args.temperature
            if best_score < args.target:
                break
        if best_score < args.target:
            break
        print(
            f"restart={restart + 1}/{args.restarts} best={best_score} "
            f"accepted={accepted}/{attempted}"
        )

    ordered = sorted(best_members, key=lambda i: vertices[i])
    print(
        f"best boundary={best_score};",
        f"connected={connected(best_members, adjacency, ordered[0])}",
    )
    print("vertices:")
    for vertex in ordered:
        print("  " + "".join(str(symbol) for symbol in vertices[vertex]))
    return 0


def main() -> int:
    """Parse command-line options and launch the search."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, default=10)
    parser.add_argument("--k", type=int, default=5)
    parser.add_argument("--size", type=int, default=26)
    parser.add_argument("--steps", type=int, default=100_000)
    parser.add_argument("--restarts", type=int, default=8)
    parser.add_argument("--temperature", type=float, default=3.0)
    parser.add_argument("--cooling", type=float, default=0.9999)
    parser.add_argument("--target", type=int, default=298)
    parser.add_argument("--seed", type=int, default=20260916)
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
