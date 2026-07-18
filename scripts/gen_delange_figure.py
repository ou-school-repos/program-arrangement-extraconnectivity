import math
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

maxexp = 20
E_running = 0
xs, ys = [], []
for n in range(1, 2**maxexp + 1):
    E_running += bin(n-1).count('1')
    if n >= 256:
        lg = math.log2(n)
        xs.append(lg - math.floor(lg))
        ys.append(E_running/n - 0.5*lg)

fig, axes = plt.subplots(2, 1, figsize=(6.5, 6.4))
ax = axes[0]
ax.plot(xs, ys, ',', markersize=0.5, color='#1a4a7a', alpha=0.35, rasterized=True)
ax.set_xlabel(r'$\{\log_2 R\}$  (fractional part)')
ax.set_ylabel(r'$\Phi = \Eseq(R)/R - \frac{1}{2}\log_2 R$'.replace('\\Eseq','s'))
ax.set_title(r'Delange fluctuation $\Phi(\log_2 R)$ of $s(R)$ (A000788)')
ax.grid(alpha=0.25)

Rmax = 1 << 15
Ev = [0]*(Rmax+1)
for n in range(1, Rmax+1): Ev[n] = Ev[n-1] + bin(n-1).count('1')
Rs = list(range(2, Rmax+1))
Cv, Xv = [], []
for R in Rs:
    d = (R-1).bit_length()
    Cr = R*(d+1) - 2**d - Ev[R]
    Cv.append((Cr/R) - 0.5*math.log2(R))   # centered like Phi for comparability
    Xv.append((Cr - Ev[R])/R)
ax = axes[1]
ax.plot(Rs, Xv, '-', lw=0.6, color='#1a6a3a', label=r'$X_{\mathrm{HB}}(R)/R$')
ax.plot(Rs, Cv, '-', lw=0.6, color='#7a2a1a', alpha=0.8,
        label=r'$\tau_c(R)/R - \frac{1}{2}\log_2 R$')
ax.axhline(0.5, color='gray', lw=0.7, ls='--')
ax.text(3, 0.512, r'$\sup = 1/2$ (alternating-binary $R$)', fontsize=7.5, color='gray')
for d in range(2, 16):
    ax.axvline(2**d, color='gray', lw=0.3, alpha=0.4)
ax.set_xscale('log', base=2)
ax.set_xlabel(r'$R$ (log scale; gridlines at $2^d$)')
ax.set_title(r'Bounded oscillation of the correction densities')
ax.legend(fontsize=8, loc='lower right')
ax.grid(alpha=0.25)
plt.tight_layout()
plt.savefig('delange-structure.pdf', dpi=200)
plt.savefig('delange-structure.png', dpi=140)
print("ok")
