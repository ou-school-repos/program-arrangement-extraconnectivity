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

The validator automatically resumes from `CURRENT` when a disk-backed prefix
already contains a committed checkpoint. A separate interruption
test target remains available. The support remains draft infrastructure until the smoke and
end-to-end recovery tests are run successfully on the current build.

Partitioned layer execution is available with
`--partition-index P --partition-count C`. Workers read the same active
checkpoint and write `frontier.chunk.P.delta` plus validated chunk metadata.
The coordinator merges all chunks with:

```text
bin/merge_frontier_chunks n k checkpoint-prefix --partition-count C
```

Only the coordinator publishes the canonical `frontier.delta` and advances
`CURRENT`. A worker never advances the generation on its own.
