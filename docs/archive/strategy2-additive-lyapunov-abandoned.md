# Strategy 2: Additive Fiber-Size Lyapunov Potential (Abandoned)

**Status:** Dead (Algebraic inconsistency and blind to cross-fiber collisions).

An attempt was made to prove the isoperimetric inequality on $A(n,k)$ using an additive potential function over individual fiber sizes:
$$\Psi_f(V') = \sum_{\text{fibers } F} f(|V' \cap F|) = \sum_{c} w_c N_c(V')$$

The goal was to bound the target $\Phi(V') = X(V') + (m+1)D(V') \le \Psi_f(V') \le C(R) + mE(R)$.

### The Algebraic Collapse

The LP feasibility check (`lyapunov_profile_check.py`, now archived in this directory) revealed that the rigid $C(R) + mE(R)$ upper bound completely shatters the additive model before it even encounters collisions:

1. **R=1 (Single Vertex):** $C(1) + mE(1) = 0$. The vertex occupies $k$ fibers of size 1. Thus, $k \cdot w_1 \le 0 \implies w_1 = 0$.
2. **R=2 (Single Edge):** $C(2) + mE(2) = m+1$. The edge occupies one fiber of size 2 and the rest size 1. Since $w_1 = 0$, $w_2 \le m+1$. But $\Phi(R=2) = m+1$, forcing $w_2 \ge m+1$. Thus, $w_2 = m+1$.
3. **R=3:** The true target $C(3) + mE(3)$ is strictly larger than $2(m+1)$. But an additive potential constructed from 1- and 2- occupied roots would rigidly predict $2 w_2 = 2(m+1)$, causing immediate algebraic inconsistency (as explicitly diagnosed for $A(5,3)$ and $A(6,3)$).

### The Coup de Grâce: The 4-Cycle

Even if the algebraic normalizations could be relaxed, an additive function over individual fibers is inherently blind to cross-fiber collisions ($X$).
For a 4-cycle, the locked potential evaluates to exactly $\Psi_f = 4w_2 = 4m + 4$.
However, the adversarial load $\Phi$ involves 4 fibers of size 2 ($D=4$) and shared external neighbors at the corners ($X = 4(m-1)$):
$$\Phi(\text{4-cycle}) = 4(m-1) + 4(m+1) = 8m$$
For $\Phi \le \Psi_f$, we need $8m \le 4m + 4 \implies m \le 1$. It fails entirely for $m \ge 2$, where collision mass shatters the potential ceiling.

### Conclusion

A valid global potential $\Psi$ cannot be the independent sum of 1D fibers. It requires a non-additive structure (e.g., $R$-dependent weights, coordinate-coupled terms, or explicit multi-root intersection terms) because the true boundary of $A(n,k)$ is irreducibly dimensional.
