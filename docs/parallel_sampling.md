# Parallel Sampling

v1.12.0-alpha adds deterministic CPU parallel sampling for SDF builders.

## Implementation

`ParallelSampling` is a small C++17 stdlib helper. It uses `std::thread`,
fixed contiguous chunks, and indexed output writes. It does not require
OpenMP, TBB, Eigen, FCL, CUDA, or the existing research core.

## Builder Use

The DenseSDF and AdaptiveBlockSDF builders use the helper when `--threads N`
is greater than one. Dense sampling writes uniform grid phi values by global
grid index. Adaptive sampling first enumerates `(block_index, local_index)`
locations, then writes each sample back to the same block and local index.

## Deterministic Failure Handling

If a worker throws, the first failing worker index is stored and rethrown after
all workers join. This avoids detached work and keeps failures visible to CLI
and tests.
