# Roadmap

## Completed Alpha Milestones

- v0.1 interface preview.
- v0.2 existing-core `.sdfbin` load/query bridge.
- v0.3 pair query API validation.
- v0.4 adaptive builder bridge.
- v0.5 CPU narrow-phase and contact reduction.
- v0.6 alpha release hardening.
- v0.7 install/package and external integration.
- v0.8 core-free standalone analytic demo backend.
- v0.9 surrogate-guided adaptive demo workflow and SVG contact visualization.
- v1.0 optional CUDA batch query backend and CPU/GPU benchmark tooling.

## v1.0.0-alpha Scope

- CPU batch signed-distance, gradient, and normal query baseline;
- optional CUDA batch query backend for analytic/demo adaptive boxes;
- deterministic benchmark point generation;
- 10k/100k/1M CPU/GPU benchmark CLI and CSV summary;
- CPU/GPU numerical alignment tests when CUDA is available;
- graceful CUDA-unavailable skip behavior.

## Future Work

- public standalone adaptive STL-to-compressed-SDF builder;
- real trained surrogate integration with public artifacts and validation cards;
- Python package;
- full low-rank compressed SDF GPU expansion;
- CUDA-accelerated pair collision pipeline;
- optional real FCL adapter;
- stricter distance and contact quality benchmarks.
