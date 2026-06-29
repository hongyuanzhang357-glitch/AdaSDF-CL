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
- v1.0.1 query mode, expansion mode, and GPU-resident expanded SDF query backend.

## v1.0.1-alpha Scope

- explicit `QueryModeConfig` for backend, expansion, block selection, resident data, and fallback;
- reusable `ExpandedSDF`, `SDFExpander`, and `QueryEngine` APIs;
- CPU direct, CPU global-expanded, and CPU block-expanded query paths;
- CUDA global-expanded and CUDA block-expanded query paths using resident GPU data;
- deterministic benchmark CLI with backend, expansion, block, memory, setup, kernel, total-time, and error columns;
- CPU/CUDA query-mode tests and graceful CUDA-unavailable skip behavior;
- documentation of the original UI CUDA dense/block-expanded reference and the v1.0.1 parity boundary.

## Future Work

- public standalone adaptive STL-to-compressed-SDF builder;
- real trained surrogate integration with public artifacts and validation cards;
- Python package;
- full low-rank compressed SDF GPU expansion and compressed-direct CUDA query;
- CUDA-accelerated pair collision pipeline;
- optional real FCL adapter;
- stricter distance and contact quality benchmarks.
