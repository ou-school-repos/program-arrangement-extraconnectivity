#!/usr/bin/env python3
"""Generate compression counterexample side-by-side diagram for A(4,2)."""

import itertools
import math

import matplotlib
import matplotlib.pyplot as plt
import networkx as nx

matplotlib.use("Agg")


def make_graph():
    """Build A(4,2): vertices are injective 2-tuples from {1,2,3,4}."""
    G = nx.Graph()
    verts = [(a, b) for a, b in itertools.permutations(range(1, 5), 2)]
    G.add_nodes_from(verts)
    for u in verts:
        for v in verts:
            if u < v and sum(1 for i in range(2) if u[i] != v[i]) == 1:
                G.add_edge(u, v)
    return G


def boundary(G, S):
    S_set = set(S)
    return {v for v in G.nodes if v not in S_set and any(u in S_set for u in G[v])}


def draw_panel(ax, G, pos, S, bnd, title, bnd_color, legend_label, notes):
    S_set, bnd_set = set(S), set(bnd)
    gray = [v for v in G.nodes if v not in S_set and v not in bnd_set]

    nx.draw_networkx_edges(G, pos, ax=ax, alpha=0.12, width=0.5)

    bnd_edges = [
        (u, v)
        for u, v in G.edges
        if (u in S_set and v in bnd_set) or (v in S_set and u in bnd_set)
    ]
    nx.draw_networkx_edges(
        G, pos, edgelist=bnd_edges, ax=ax, alpha=0.45, width=1.3, edge_color=bnd_color
    )

    int_edges = [(u, v) for u, v in G.edges if u in S_set and v in S_set]
    if int_edges:
        nx.draw_networkx_edges(
            G,
            pos,
            edgelist=int_edges,
            ax=ax,
            alpha=0.8,
            width=2.5,
            edge_color="#4a90d9",
        )

    nx.draw_networkx_nodes(
        G,
        pos,
        nodelist=gray,
        node_color="#e8e8e8",
        edgecolors="#bbb",
        linewidths=0.6,
        node_size=320,
        ax=ax,
    )
    nx.draw_networkx_nodes(
        G,
        pos,
        nodelist=list(bnd),
        node_color=bnd_color,
        edgecolors="black",
        linewidths=0.8,
        node_size=340,
        ax=ax,
    )
    nx.draw_networkx_nodes(
        G,
        pos,
        nodelist=list(S),
        node_color="#4a90d9",
        edgecolors="#1a3a6a",
        linewidths=1.5,
        node_size=400,
        ax=ax,
    )

    labels = {v: f"{v[0]},{v[1]}" for v in G.nodes}
    for v in G.nodes:
        color = "white" if v in S_set or v in bnd_set else "#444"
        nx.draw_networkx_labels(
            G,
            pos,
            {v: labels[v]},
            font_size=7,
            font_weight="bold",
            font_color=color,
            ax=ax,
        )

    ax.set_title(title, fontsize=10, fontweight="bold", pad=6)
    ax.margins(0.15)
    ax.axis("off")

    # Notes below panel
    for i, note in enumerate(notes):
        ax.text(
            0.5,
            -0.05 - i * 0.06,
            note,
            transform=ax.transAxes,
            ha="center",
            fontsize=7,
            style="italic",
            color="#555",
        )

    from matplotlib.patches import Patch

    legend_items = [
        Patch(facecolor="#4a90d9", edgecolor="#1a3a6a", label=f"Set ({len(S)})"),
        Patch(
            facecolor=bnd_color, edgecolor="black", label=f"{legend_label} ({len(bnd)})"
        ),
    ]
    ax.legend(
        handles=legend_items,
        loc="upper left",
        ncol=1,
        fontsize=7,
        framealpha=0.85,
    )


def main():
    G = make_graph()
    verts_sorted = sorted(G.nodes)
    pos = {}
    for i, v in enumerate(verts_sorted):
        angle = 2 * math.pi * i / len(verts_sorted) - math.pi / 2
        pos[v] = (math.cos(angle), math.sin(angle))

    V_before = [(4, 3), (1, 3)]
    bnd_before = boundary(G, V_before)
    V_after = [(4, 1), (1, 3)]
    bnd_after = boundary(G, V_after)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 6.5), dpi=100)

    # Suptitle
    fig.suptitle(
        "Compression Counterexample in A(4,2)\n"
        "Vertex isoperimetry via shift operators fails for partial permutations",
        fontsize=12,
        fontweight="bold",
        y=0.98,
    )

    draw_panel(
        ax1,
        G,
        pos,
        V_before,
        bnd_before,
        "BEFORE:  V' = {[4,3], [1,3]}",
        "#e8910c",
        "∂V'",
        ["[4,3]↔[1,3] adjacent (share symbol 3)", "∂V' = 5 boundary vertices"],
    )

    # Arrow between panels
    fig.text(
        0.5,
        0.52,
        "shift\n3 → 1\n⟹",
        ha="center",
        va="center",
        fontsize=13,
        fontweight="bold",
        color="#666",
        transform=fig.transFigure,
    )

    draw_panel(
        ax2,
        G,
        pos,
        V_after,
        bnd_after,
        "AFTER:  C(V') = {[4,1], [1,3]}",
        "#d63031",
        "∂C(V')",
        ["[4,3]→[4,1]: internal edge destroyed!", "∂C(V') = 7 — boundary INCREASED"],
    )

    fig.tight_layout(rect=[0, 0.10, 1, 0.92], w_pad=3.5)

    # Explanation footer
    explanation = (
        "Each vertex is an injective 2-symbol sequence from {1,2,3,4}.  "
        "Edges connect sequences differing at exactly 1 position.\n"
        "Blue = chosen set V'.  "
        "Orange/Red = external boundary (adjacent to V' but not in V').  "
        "Gray = non-adjacent to V'.\n"
        "Compression replaces symbol 3→1 where legal.  "
        "[1,3] cannot shift (already uses 1).  "
        "Result: boundary grows from 5→7.  ∎"
    )
    fig.text(
        0.5,
        0.02,
        explanation,
        ha="center",
        va="bottom",
        fontsize=8,
        color="#333",
        linespacing=1.5,
        bbox=dict(
            boxstyle="round,pad=0.4", facecolor="#f5f5f5", edgecolor="#ccc", alpha=0.9
        ),
        transform=fig.transFigure,
    )

    out = "docs/compression_counterexample.png"
    fig.savefig(out, dpi=120, bbox_inches="tight", pad_inches=0.15, facecolor="white")
    plt.close(fig)

    import os

    size_kb = os.path.getsize(out) // 1024
    print(f"Saved {out} ({size_kb}KB)")


if __name__ == "__main__":
    main()
