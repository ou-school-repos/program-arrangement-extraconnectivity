#!/usr/bin/env python3
"""Generate complexity-curves.pdf — vector version of the growth comparison plot.

Uses actual measured wall times from README.md brute-force logs.
Fits log10(T) = a + b*(R-2)*log10(R) to the search data.
Predictor measured as <2ms for all R up to 128.
"""

import numpy as np
from scipy.optimize import curve_fit
import matplotlib
matplotlib.use("pdf")
import matplotlib.pyplot as plt

# ── Actual measured search times from README.md ───────────────────
# R=2..5 logged as 0.000s; estimate from Gen counts (README) / ~5M gens/s
search_R = np.array([2,        3,        4,        5,        6,     7,     8,     9,      10])
search_T = np.array([0.000001, 0.000002, 0.00001,  0.0002,   0.004, 0.072, 1.782, 74.553, 4140.057])
# R=2: 1 gen, R=3: 6 gen, R=4: 46 gen, R=5: 1102 gen (all <1ms)
# R=6: 0.004s, R=7: 0.072s, R=8: 1.782s, R=9: 74.553s, R=10: 4140s
# R>=11 never completed (intractable).

# ── Best fit: log10(T) = a + b*(R-2)*log10(R) ────────────────────
log_T = np.log10(search_T)
x_model = (search_R - 2) * np.log10(search_R)

def linear_model(x, a, b):
    return a + b * x

popt, _ = curve_fit(linear_model, x_model, log_T)
a_fit, b_fit = popt

print(f"Best fit: log10(T) = {a_fit:.4f} + {b_fit:.4f} * (R-2)*log10(R)")
print(f"  i.e.  T ≈ 10^{a_fit:.2f} · R^({b_fit:.2f}·(R-2))")

# ── Generate fitted curve ────────────────────────────────────────
R_smooth = np.linspace(2, 12, 500)
x_smooth = (R_smooth - 2) * np.log10(R_smooth)
T_fit = 10 ** (a_fit + b_fit * x_smooth)

# ── Predictor: O(R^4) ────────────────────────────────────────────
# Process startup (~1.4ms) dominates, masking actual compute time.
# Scale so curve passes through ~1ms at R=10 (measured floor).
pred_c = 0.001 / (10 ** 4)
T_pred = pred_c * R_smooth ** 4

# ── Plot ──────────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(6.5, 3.2))

# Search: data points + best fit
ax.semilogy(R_smooth, T_fit, "r-", linewidth=2,
            label=rf"Exhaustive Search (fit: $R^{{{b_fit:.1f}(R-2)}}$)")
ax.semilogy(R_smooth[R_smooth > 10], T_fit[R_smooth > 10], "r--",
            linewidth=1.5, alpha=0.5)
ax.plot(search_R, search_T, "ro", markersize=5, zorder=5)

# Predictor: O(R^4)
ax.semilogy(R_smooth, T_pred, "b-", linewidth=2,
            label=r"Hamming Predictor ($O(R^4)$)")

# Fill gap
ax.fill_between(R_smooth, T_pred, T_fit, alpha=0.12, color="red",
                where=(T_fit > T_pred))

ax.set_xlabel("Subgraph Size ($R$)")
ax.set_ylabel("Wall Time (Seconds)")
ax.set_title("Search vs. Predictor Complexity (Log Scale)")
ax.legend(fontsize=9, loc="upper left")
ax.set_xlim(1.5, 12.5)
ax.set_ylim(1e-7, 5e7)
ax.grid(True, alpha=0.3)

# Annotate intractable region
ax.annotate("intractable", xy=(11.5, T_fit[R_smooth >= 11.5][0]),
            fontsize=9, color="red", ha="center", va="bottom", style="italic")

plt.tight_layout()
fig.savefig("../docs/complexity-curves.pdf", bbox_inches="tight")
print("Wrote docs/complexity-curves.pdf")
