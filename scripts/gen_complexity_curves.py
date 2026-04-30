#!/usr/bin/env python3
"""Generate complexity-curves.pdf — vector version of the growth comparison plot.

Uses actual measured wall times from the exhaustive search (README.md).
The predictor curve is theoretical O(R^4), scaled to match measured sub-ms times.
"""

import numpy as np
from scipy.optimize import curve_fit
import matplotlib
matplotlib.use("pdf")
import matplotlib.pyplot as plt

# ── Actual measured search times from README.md ───────────────────
# R=2..5 rounded to 0 in logs; use Gen counts as proxy for sub-ms
search_R = np.array([6,     7,     8,     9,      10])
search_T = np.array([0.004, 0.072, 1.782, 74.553, 4140.057])
# From README.md brute-force logs. R>=11 never completed (intractable).

# ── Fit a * R^(R-2) to search data ───────────────────────────────
def search_model(r, a):
    return a * r ** (r - 2)

popt_s, _ = curve_fit(search_model, search_R, search_T, p0=[1e-5])
a_fit = popt_s[0]

# ── Predictor: measured as <2ms for all R up to 128 ──────────────
# Use theoretical O(R^4) scaled so that R=128 ≈ 0.002s
b_fit = 0.002 / (128 ** 4)

print(f"Search Model: T(R) = {a_fit:.6e} * R^(R-2)")
print(f"Predictor Model: T(R) = {b_fit:.6e} * R^4")

# ── Plot ──────────────────────────────────────────────────────────
R = np.linspace(2, 12, 500)
T_search = a_fit * R ** (R - 2)
T_predict = b_fit * R ** 4

fig, ax = plt.subplots(figsize=(6.5, 3.2))

ax.semilogy(R, T_search, "r-", linewidth=2, label=r"Exhaustive Search ($R^{R-2}$)")
ax.semilogy(R, T_predict, "b-", linewidth=2, label=r"Hamming Predictor ($R^4$)")
ax.fill_between(R, T_predict, T_search, alpha=0.15, color="red")

# Mark actual measured data points
ax.plot(search_R, search_T, "ro", markersize=4, zorder=5)

ax.set_xlabel("Subgraph Size ($R$)")
ax.set_ylabel("Wall Time (Seconds)")
ax.set_title("Search vs. Predictor Complexity (Log Scale)")
ax.legend(fontsize=10, loc="center right")
ax.set_xlim(2, 12)
ax.grid(True, alpha=0.3)

plt.tight_layout()
fig.savefig("../docs/complexity-curves.pdf", bbox_inches="tight")
print("Wrote docs/complexity-curves.pdf")
