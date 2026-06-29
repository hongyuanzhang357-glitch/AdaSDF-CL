# AdaSDF-CL v1.0.1-alpha

## Highlights

- Added explicit query-mode configuration for backend, expansion mode, block
  selection, resident expanded data, and CPU fallback policy.
- Added `ExpandedSDF`, `SDFExpander`, and `QueryEngine`.
- Added CPU direct, CPU global-expanded, CPU block-expanded, CUDA
  global-expanded, and CUDA block-expanded query paths.
- Added CUDA resident expanded SDF upload/query/release support for pre-expanded
  global and block dense data.
- Added `adasdf_query_mode_demo`.
- Extended `adasdf_collide` with `--backend`, `--expansion`, and `--blocks`
  request wiring.
- Extended `adasdf_benchmark_batch_query` with query backend, expansion mode,
  selected blocks, memory, setup, kernel, total-time, fallback, and error
  columns.
- Added query-mode and CUDA resident backend documentation.

## Validation

- CPU Release build: PASS.
- CPU CTest: 37/37 PASS.
- CUDA Release build: PASS when built from an ASCII-only source/build path.
- CUDA CTest: 37/37 PASS.
- Query mode CLI smoke tests: PASS.
- `CUDA + None` rejection: PASS with
  `CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.`
- Repository clean check: expected PASS before tagging.

## Important Boundaries

- CUDA is optional and disabled by default.
- CUDA compressed-direct query is not implemented.
- CUDA query requires `Global` or `Block` expansion.
- CUDA pair collision is not implemented in this release; collision requests
  now carry query-mode intent and validation, then continue through the existing
  CPU narrow-phase.
- Expanded-grid distances and finite-difference normals are approximation
  outputs, not certified contact-quality claims.
- Full low-rank compressed SDF GPU expansion remains future work.

## Benchmark Command Examples

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cpu --expansion none --out cpu_direct.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion global --out cuda_global.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion block --blocks 0,1,2 --out cuda_blocks.csv
```

Representative local Release rows:

| Backend | Expansion | Blocks | Points | Total ms | ns/query | GPU MB | Status |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| CPU | None | all | 100,000 | 5.0764 | 50.764 | 0 | ok |
| CUDA | Global | all | 100,000 | 5.2922 | 52.922 | 2.0001 | ok |
| CUDA | Global | all | 1,000,000 | 40.7194 | 40.7194 | 2.0001 | ok |
| CUDA | Block | 0,1,2 | 1,000,000 | 59.8738 | 59.8738 | 0.7502 | ok |

## Suggested Tag

```text
v1.0.1-alpha
```
