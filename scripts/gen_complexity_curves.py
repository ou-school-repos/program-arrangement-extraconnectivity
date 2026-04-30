#!/usr/bin/env python3
"""Generate complexity-curves.pdf.

Uses actual measured wall times from README.md (search) and timed
predict binary runs (predictor).
Fits log10(T) = a + b*(R-2)*log10(R) to search data.
"""

import matplotlib
import numpy as np
from scipy.optimize import curve_fit

matplotlib.use("pdf")
import matplotlib.pyplot as plt  # noqa: E402

# ── Actual measured search times from README.md ──────────────
search_R = np.array(
    [2, 3, 4, 5, 6, 7, 8, 9, 10]
)
search_T = np.array(
    [0.000001, 0.000002, 0.00001, 0.0002, 0.004,
     0.072, 1.782, 74.553, 4140.057]
)
# R=2: 1 gen, R=3: 6 gen, R=4: 46 gen, R=5: 1102 gen (all <1ms)
# R=6..10: actual wall clock from README.md brute-force logs.
# R>=11 never completed (intractable).

# ── Predictor times: ./predict --verify, 100 iters, overhead sub'd ─
pred_R = np.array(
    [4, 6, 8, 10, 12, 16, 20, 25, 30, 35, 40]
)
pred_T = np.array(
    [0.099, 0.240, 0.205, 0.581, 1.194,
     2.739, 5.349, 9.035, 15.66, 24.80, 39.80]
)
pred_T = pred_T / 1000.0  # convert ms -> seconds


# ── Best fit search: log10(T) = a + b*(R-2)*log10(R) ────────
log_T = np.log10(search_T)
x_model = (search_R - 2) * np.log10(search_R)


def linear_model(x, a, b):
    return a + b * x


popt, _ = curve_fit(linear_model, x_model, log_T)
a_fit, b_fit = popt
print(
    f"Search fit: log10(T) = {a_fit:.4f}"
    f" + {b_fit:.4f} * (R-2)*log10(R)"
)

# ── Best fit predictor: log10(T) = c + d*log10(R) ───────────
log_pred_T = np.log10(pred_T)
log_pred_R = np.log10(pred_R)
popt_p, _ = curve_fit(linear_model, log_pred_R, log_pred_T)
c_fit, d_fit = popt_p
print(f"Predictor fit: T ~ R^{d_fit:.2f}")

# ── Generate curves ─────────────────────────────────────────
R_smooth = np.linspace(2, 12, 500)
x_smooth = (R_smooth - 2) * np.log10(R_smooth)
T_search_fit = 10 ** (a_fit + b_fit * x_smooth)
T_pred_fit = 10 ** (c_fit + d_fit * np.log10(R_smooth))

# ── Plot ─────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(6.5, 3.2))

# Search: data points + fit
ax.semilogy(
    R_smooth, T_search_fit, "r-", linewidth=2,
    label=rf"Exhaustive Search (fit: $R^{{{b_fit:.1f}(R-2)}}$)",
)
ax.semilogy(
    R_smooth[R_smooth > 10], T_search_fit[R_smooth > 10],
    "r--", linewidth=1.5, alpha=0.5,
)
ax.plot(search_R, search_T, "ro", markersize=5, zorder=5)

# Predictor: data points + fit
ax.semilogy(
    R_smooth, T_pred_fit, "b-", linewidth=2,
    label=rf"Hamming Predictor (fit: $R^{{{d_fit:.1f}}}$)",
)
ax.plot(
    pred_R[pred_R <= 12], pred_T[pred_R <= 12],
    "bs", markersize=4, zorder=5,
)

# Fill gap
ax.fill_between(
    R_smooth, T_pred_fit, T_search_fit,
    alpha=0.12, color="red",
    where=(T_search_fit > T_pred_fit),
)

ax.set_xlabel("Subgraph Size ($R$)")
ax.set_ylabel("Wall Time (Seconds)")
ax.set_title("Search vs. Predictor Complexity (Log Scale)")
ax.legend(fontsize=9, loc="upper left")
ax.set_xlim(1.5, 12.5)
ax.set_ylim(1e-7, 5e7)
ax.grid(True, alpha=0.3)

# Annotate intractable region
ax.annotate(
    "intractable",
    xy=(11.5, T_search_fit[R_smooth >= 11.5][0]),
    fontsize=9, color="red", ha="center",
    va="bottom", style="italic",
)

plt.tight_layout()
fig.savefig("../docs/complexity-curves.pdf", bbox_inches="tight")
print("Wrote docs/complexity-curves.pdf")
