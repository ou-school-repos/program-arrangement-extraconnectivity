"""Machine verification of every identity in paper Section: Analytic
Structure of the Boundary Coefficients (gf-section.tex).
Series identities to order 600; closed forms and density decomposition to R < 2^16.
Run: python3 verify_gf_identities.py  — all lines must print True."""

import math
import sys

all_ok = True


def check(label, ok):
    global all_ok
    all_ok = all_ok and ok
    print(label, ok)


def popcount(x):
    return bin(x).count("1")


def bl(x):
    return x.bit_length()


def E(N):
    return sum(popcount(i) for i in range(N))


def sbl(N):
    return sum(bl(i) for i in range(N))


def C(R):
    return 0 if R == 0 else (R - 1) + sbl(R) - E(R)


# --- Prop A: closed forms ---
ok = True
for N in range(1, 4097):
    L = bl(N - 1)
    if sbl(N) != N * L - 2**L + 1:
        ok = False
        print("sbl FAIL", N)
check("A1 sbl(N)=N*L-2^L+1:", ok)
ok = True
for R in range(1, 4097):
    d = bl(R - 1)
    if C(R) != R * (d + 1) - 2**d - E(R):
        ok = False
        print("C FAIL", R)
check("A2 C(R)=R(d+1)-2^d-E(R):", ok)
# corollaries
ok = all(C(2**d) == E(2**d) for d in range(0, 13))
check("A3 C(2^d)=E(2^d):", ok)
ok = all(
    (C(R) - E(R)) == R * (bl(R - 1) + 1) - 2 ** bl(R - 1) - 2 * E(R)
    for R in range(1, 2049)
)
check("A4 X_HB(R)=R(d+1)-2^d-2E(R):", ok)

# --- Prop B: OGFs (truncated series to degree M) ---
M = 600


def series_mul(a, b):
    c = [0] * (M + 1)
    for i, ai in enumerate(a):
        if ai:
            for j in range(0, M + 1 - i):
                c[i + j] += ai * b[j]
    return c


one_minus_x_inv = [1] * (M + 1)  # 1/(1-x)


def geom(period_start, step):  # x^a/(1-x^b)
    c = [0] * (M + 1)
    t = period_start
    while t <= M:
        c[t] += 1
        t += step
    return c


# S(x) = sum_j x^{2^j}/((1-x)(1+x^{2^j}))
#      -> use x^{2^j}/(1+x^{2^j}) = sum_{t odd} (-1)^{t-1}...
# easier: x^{2^j}/(1+x^{2^j}) = x^{2^j} - x^{2*2^j} + x^{3*2^j} - ...
def alt(pow2):
    c = [0] * (M + 1)
    t = pow2
    s = 1
    while t <= M:
        c[t] += s
        s = -s
        t += pow2
    return c


inner = [0] * (M + 1)
j = 0
while 2**j <= M:
    a = alt(2**j)
    for i in range(M + 1):
        inner[i] += a[i]
    j += 1
S = series_mul(one_minus_x_inv, inner)
ok = all(S[n] == popcount(n) for n in range(0, M + 1))
check("B1 OGF s2: S(x)=1/(1-x) * sum x^{2^j}/(1+x^{2^j}):", ok)
xf = [0] * (M + 1)
xf[1] = 1
FE = series_mul(series_mul(xf, one_minus_x_inv), S)  # x/(1-x)*S = sum E(N)x^N
ok = all(FE[n] == E(n) for n in range(0, M + 1))
check("B2 OGF E_seq: F_E=(x/(1-x)^2)*sum x^{2^j}/(1+x^{2^j}):", ok)
innerL = [0] * (M + 1)
j = 0
while 2**j <= M:
    t = 2**j
    innerL[t] += 1
    j += 1
FL = series_mul(series_mul(series_mul(xf, one_minus_x_inv), one_minus_x_inv), innerL)
ok = all(FL[n] == sbl(n) for n in range(0, M + 1))
check("B3 OGF sum_bit_length: F_L=(x/(1-x)^2)*sum x^{2^j}:", ok)
# F_C = x^2/(1-x)^2 + (x/(1-x)^2) * sum_j x^{2^{j+1}}/(1+x^{2^j})
inner2 = [0] * (M + 1)
j = 0
while 2**j <= M:
    b = 2**j
    # x^{2b}/(1+x^b) = x^{2b} - x^{3b} + x^{4b} - ...
    t = 2 * b
    s = 1
    while t <= M:
        inner2[t] += s
        s = -s
        t += b
    j += 1
x2 = [0] * (M + 1)
x2[2] = 1
FC1 = series_mul(series_mul(x2, one_minus_x_inv), one_minus_x_inv)
FC2 = series_mul(series_mul(series_mul(xf, one_minus_x_inv), one_minus_x_inv), inner2)
ok = all(FC1[n] + FC2[n] == C(n) for n in range(0, M + 1))
check("B4 OGF C_constant:", ok)

# --- Prop C: Mahler equation S(x)=(1+x)S(x^2)+x/(1-x^2) ---
S2 = [0] * (M + 1)
for i in range(0, M // 2 + 1):
    S2[2 * i] = S[i]
lhs = S
rhs = [0] * (M + 1)
for i in range(M + 1):
    rhs[i] += S2[i]
    if i >= 1:
        rhs[i] += S2[i - 1]
t = 1
while t <= M:
    rhs[t] += 1
    t += 2  # x/(1-x^2)
ok = all(lhs[n] == rhs[n] for n in range(M + 1))
check("C1 Mahler: S(x)=(1+x)S(x^2)+x/(1-x^2):", ok)
# F_E version: x*F_E(x) = (1+x)^2 * F_E(x^2) + x^3/((1-x)(1-x^2))
FE2 = [0] * (M + 1)
for i in range(0, M // 2 + 1):
    FE2[2 * i] = FE[i]
lhs2 = [0] * (M + 1)
for i in range(M):
    lhs2[i + 1] = FE[i]
opx = [0] * (M + 1)
opx[0] = 1
opx[1] = 2
opx[2] = 1  # (1+x)^2
rhs2 = series_mul(opx, FE2)
x3 = [0] * (M + 1)
x3[3] = 1
tail = series_mul(series_mul(x3, one_minus_x_inv), geom(0, 2))
for i in range(M + 1):
    rhs2[i] += tail[i]
ok = all(lhs2[n] == rhs2[n] for n in range(M + 1))
check("C2 Mahler for F_E: x*F_E(x)=(1+x)^2 F_E(x^2)+x^3/((1-x)(1-x^2)):", ok)

# --- Prop D: Delange fluctuation, numeric ---
# Phi(N) := E(N)/N - (1/2)log2(N)  should be a bounded periodic-in-log2 wave
vals = []
N = 1
Es = 0
i = 0
maxN = 2**20
E_running = 0
phi_samples = []
for n in range(1, maxN + 1):
    E_running += popcount(n - 1)
    if n >= 16 and (n & 0xF) == 0:  # sample
        phi_samples.append(E_running / n - 0.5 * math.log2(n))

print(
    "D  Delange wave: min=%.6f max=%.6f (bounded, nonconstant)"
    % (min(phi_samples), max(phi_samples))
)

Ev = [0]
for n in range(1, 1 << 16):
    Ev.append(Ev[-1] + bin(n - 1).count("1"))


def C(R):
    d = (R - 1).bit_length()
    return R * (d + 1) - 2**d - Ev[R]


ok = True
for R in range(2, 1 << 16):
    lg = math.log2(R)
    u = lg - math.floor(lg)
    Psi = 2 - u - 2 ** (1 - u)
    Phi = Ev[R] / R - 0.5 * lg  # Delange fluctuation, exact by definition
    # claimed exact: C(R)/R = (1/2)log2 R + Psi(u) - Phi(log2 R)
    if abs(C(R) / R - (0.5 * lg + Psi - Phi)) > 1e-9:
        ok = False
        print("FAIL", R)
        break
check("C(R)/R = (1/2)log2R + Psi({log2R}) - Phi(log2R):", ok)
# X_HB/R = Psi - 2Phi bounded, zero at powers of two, positive elsewhere
Rs_D = list(range(2, 1 << 16))
X = [((C(R) - Ev[R]) / R) for R in Rs_D]
print("X/R range: [%.4f, %.4f]" % (min(X), max(X)))
check(
    "X(2^d)=0 for all sampled powers of two:",
    all(C(1 << d) == Ev[1 << d] for d in range(1, 16)),
)
check(
    "X(R)>0 for all non-powers-of-two in range:",
    all(x > 0 for R, x in zip(Rs_D, X) if R & (R - 1) != 0),
)

if not all_ok:
    print("FAILURE: one or more identities did not verify")
    sys.exit(1)
