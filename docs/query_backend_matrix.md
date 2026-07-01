# Query Backend Matrix

| Backend | Expansion | Supported | Output | Notes |
| --- | --- | --- | --- | --- |
| CPU | None | yes | phi, normal | Direct model query; default stable path. |
| CPU | Global | yes | phi, normal | Pre-expanded dense layout; validates expanded behavior. |
| CPU | Block | yes | phi, normal | All or selected blocks; useful for local contact regions. |
| CUDA | None | no | - | CUDA requires pre-expanded data. |
| CUDA | Global | yes | phi, normal / phi-only | Resident expanded layout; optional CUDA. |
| CUDA | Block | yes | phi, normal / phi-only | Selected block support; point distribution matters. |

CPU direct is the default most stable path. CPU expanded modes validate and
simulate GPU layouts without requiring CUDA. CUDA only runs expanded layouts.
Global expansion is best for global batch queries. Block expansion is best for
local contact regions.

DenseSDF models built with `adasdf_build_dense_sdf` support the same direct CPU
query path and can be expanded by `SDFExpander` for CPU/CUDA expanded-query
experiments.

`--output phi` is closest to the original UI kernel-only comparison.
`--output phi,normal` is closer to contact workflow needs. `--device-only` is
only for performance upper-bound timing and is not a correctness mode.
