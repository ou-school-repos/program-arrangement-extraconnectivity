# Star-sweep checkpoint support

This directory contains the storage boundary for the
`validate_extra_cut_*` family. It is intentionally separate from the generic
arrangement and bitmap utilities.

The intended checkpoint protocol is:

1. Finish a level-synchronous BFS expansion.
2. Serialize the immutable `next_frontier` delta to a temporary file.
3. `fsync` and atomically publish the delta.
4. Write and durably publish generation metadata containing the graph
   signature and BFS state.
5. Atomically advance `CURRENT`.
6. Merge and reuse the working RAM/mmap bitmaps.

The working bitmaps are not themselves the durable checkpoint. A resume path
must reconstruct them from committed frontier deltas and reject any generation
whose signature, sizes, or metadata do not match the requested `A(n,k)`.

The validator now has opt-in `--resume` integration and a separate interruption
test target. The support remains draft infrastructure until the smoke and
end-to-end recovery tests are run successfully on the current build.
