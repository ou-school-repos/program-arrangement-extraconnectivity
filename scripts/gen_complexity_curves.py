#!/usr/bin/env python3
"""Generate complexity-curves.pdf — vector version of the growth comparison plot.

Reproduces the original Wolfram script (best-fit-curve.wls) using the same
measured data points and fitting a*R^(R-2) and b*R^4.
"""

import numpy as np
from scipy.optimize import curve_fit
import matplotlib
matplotlib.use("pdf")
import matplotlib.pyplot as plt

# ── Actual measured data from brute-force search runs ──────────────
search_R = np.array([6, 7, 8, 9, 10])
search_T = np.array([0.058, 3.353, 304.064, 4320.0, 4140.057])

predictor_R = np.array([6, 8, 10, 16, 32])
predictor_T = np.array([0.0001, 0.0002, 0.0003, 0.0005, 0.002])

# ── Fit a * R^(R-2) to search data ────────────────────────────────
def search_model(r, a):
    return a * r ** (r - 2)

def predictor_model(r, b):
    return b * r ** 4

popt_s, _ = curve_fit(search_model, search_R, search_T, p0=[1e-5])
popt_p, _ = curve_fit(predictor_model, predictor_R, predictor_T, p0=[1e-10])

a_fit = popt_s[0]
b_fit = popt_p[0]

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

# Mark actual data points
ax.plot(search_R, search_T, "ro", markersize=4, zorder=5)
ax.plot(predictor_R[:3], predictor_T[:3], "bo", markersize=4, zorder=5)

ax.set_xlabel("Subgraph Size ($R$)")
ax.set_ylabel("Wall Time (Seconds)")
ax.set_title("Search vs. Predictor Complexity (Log Scale)")
ax.legend(fontsize=10, loc="center right")
ax.set_xlim(2, 12)
ax.grid(True, alpha=0.3)

plt.tight_layout()
fig.savefig("../docs/complexity-curves.pdf", bbox_inches="tight")
print("Wrote docs/complexity-curves.pdf")
