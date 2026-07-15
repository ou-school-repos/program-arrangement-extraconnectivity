import Mathlib.Combinatorics.SetFamily.KruskalKatona
import Mathlib.Combinatorics.SetFamily.Shadow
import Arrangement.ArrDefs
import Arrangement.ArrangementExtraconnectivity

open Finset

/--
  The support of a single Arrangement Graph vertex.
  Since `v` is an injective sequence of length `k` mapping `Fin k` to `Fin n`,
  its image (range of active symbols) is a subset of `Fin n` of size exactly `k`.
-/
def vertex_support {n k : ℕ} (v : ArrVertex n k) : Finset (Fin n) :=
  (univ : Finset (Fin k)).image v.val

/--
  The cardinality of the support of any arrangement vertex `v` is exactly `k`.
  This is guaranteed by the injectivity of `v.val` (which is packaged in `v.prop`).
-/
lemma vertex_support_card {n k : ℕ} (v : ArrVertex n k) :
    (vertex_support v).card = k := by
  unfold vertex_support
  rw [card_image_of_injective _ v.prop]
  exact Fintype.card_fin k

/--
  The Support Projection of a subset of vertices V' in A(n,k).
  Maps each injective sequence `v ∈ V'` to its support set, producing
  a family of `k`-element subsets over the finite universe `Fin n`.
-/
def support_proj {n k : ℕ} (V' : Finset (ArrVertex n k)) : Finset (Finset (Fin n)) :=
  V'.image vertex_support

/--
  The support projection is a uniform set family of size `k` (Sized k).
  TODO(review): this file is only a scaffold; it does not establish the shadow
  inequality or colex correspondence claimed in the roadmap.
-/
lemma support_proj_sized {n k : ℕ} (V' : Finset (ArrVertex n k)) :
    Set.Sized k (support_proj V' : Set (Finset (Fin n))) := by
  intro s hs
  rw [Finset.mem_coe] at hs
  unfold support_proj at hs
  obtain ⟨v, _, rfl⟩ := mem_image.mp hs
  exact vertex_support_card v
