# Optional CUDA Query Backend

AdaSDF-CL v1.0.2-alpha keeps CUDA optional and provides a GPU-resident query
path for pre-expanded SDF data. It also adds reusable CUDA query workspaces and
a phi-only kernel for benchmark comparisons.

## Scope

Supported first:

- core-free `AnalyticSDFModel` boxes;
- core-free `DemoAdaptiveSDFModel` boxes;
- global dense SDF expansion;
- selected-block dense SDF expansion;
- resident GPU upload of expanded values and block metadata;
- reusable CUDA query workspace for points, phi, and normal buffers;
- batch signed distance;
- batch finite-difference normal output;
- batch phi-only output for signed-distance-only kernel timing;
- CPU/GPU numerical comparison in tests and benchmarks.

Not supported yet:

- CUDA compressed-direct query with no expansion;
- full low-rank compressed SDF GPU expansion;
- arbitrary mesh/STL GPU SDF construction;
- CUDA-accelerated pair collision;
- certified industrial GPU contact solving;
- FCL ABI compatibility.

`CUDA + None` is invalid and reports:

```text
CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.
```

The collision CLI maps the same boundary to:

```text
CUDA collision query requires pre-expanded SDF data.
```

## CMake

CUDA is disabled by default:

```bash
cmake -S . -B build -DADASDF_CL_ENABLE_CUDA=OFF
```

To request CUDA:

```bash
cmake -S . -B build-cuda -DADASDF_CL_ENABLE_CUDA=ON
```

If CUDA is requested but unavailable, configure continues and builds the CPU
runtime. The summary reports:

```text
CUDA backend: OFF
CUDA toolkit: not found
```

If CUDA is available:

```text
CUDA backend: ON
CUDA toolkit: found
```

On Windows, CUDA/MSBuild can be sensitive to non-ASCII source or build paths.
If CUDA compiler detection or `.cu` compilation fails in a path with non-ASCII
characters, validate again from an ASCII-only checkout, junction, or build
directory before treating the failure as a code issue.

## Public API

```cpp
adasdf::ExpansionOptions expansion;
expansion.expansion = adasdf::QueryExpansionMode::Global;
expansion.global_resolution = 64;

adasdf::QueryEngine engine(
    model,
    adasdf::QueryModeConfig::cudaGlobalExpanded(),
    expansion);

engine.prepare();
adasdf::BatchQueryOutput output = engine.queryBatch(points);
```

For benchmark-style repeated CUDA queries, the lower-level resident API can
reuse query buffers:

```cpp
adasdf::ExpandedSDF expanded =
    adasdf::SDFExpander::expand(*model, expansion);

adasdf::CudaResidentExpandedSDF resident;
resident.upload(expanded);

adasdf::CudaQueryWorkspace workspace;
workspace.ensureCapacity(points.size(), true);

adasdf::BatchQueryTiming timing;
adasdf::BatchQueryOutput output =
    resident.queryBatch(points, false, &workspace, &timing);
```

For signed-distance-only benchmark comparison:

```cpp
workspace.ensureCapacity(points.size(), false);
adasdf::BatchQueryOutput phi_only =
    resident.queryBatch(points, true, &workspace, &timing);
```

The phi-only output fills `signed_distances` and leaves `gradients` and
`normals` empty.

For block expansion:

```cpp
adasdf::BlockSelection blocks =
    adasdf::BlockSelection::selected({0, 1, 2});

adasdf::ExpansionOptions expansion;
expansion.expansion = adasdf::QueryExpansionMode::Block;
expansion.block_selection = blocks;
expansion.block_resolution = 32;

adasdf::QueryModeConfig config =
    adasdf::QueryModeConfig::cudaBlockExpanded(blocks);

adasdf::QueryEngine engine(model, config, expansion);
engine.prepare();
```

CPU-only users can include `<adasdf/adasdf.h>` without including CUDA headers.

## Runtime Model

`SDFExpander` first converts the model into an `ExpandedSDF`:

- `GlobalDense`: one dense grid over the model bounding box;
- `BlockDense`: one dense grid per selected block.

`CudaResidentExpandedSDF` uploads the expanded block metadata and sampled
double-precision values once. `QueryEngine::queryBatch()` then launches the
resident CUDA kernel and downloads signed distances and normals for the query
points. The SDF data is resident after upload. Query points and result buffers
are resident only when a `CudaQueryWorkspace` is reused.

The backend reports:

- expanded host memory bytes;
- resident GPU memory bytes;
- setup time;
- expansion time;
- SDF upload time;
- H2D point-copy time;
- CUDA event kernel time;
- synchronization time;
- D2H result-copy time;
- CPU postprocess time;
- total query time;
- fallback count.

`total_ms` and `kernel_ms` are intentionally different. `kernel_ms` is CUDA
event time around the kernel. `total_ms` includes allocation, point upload,
synchronization, result download, CPU postprocess, and temporary buffer free
when no reusable workspace is provided.

## Precision

The v1.0.2-alpha CUDA expanded SDF kernel uses double precision to align with
the CPU `Scalar` type. Expanded-grid distance values are approximate because
they are sampled from a finite-resolution dense grid. Normals are computed with
finite differences and are suitable for query-mode validation, not for certified
contact-normal quality claims.

`--phi-only` benchmark rows skip finite-difference normals and are the closest
AdaSDF-CL comparison point for original UI signed-distance kernel averages.

## Testing

CUDA tests are compiled in CPU-only builds and return success with a `SKIPPED`
message when CUDA is unavailable. When CUDA is available, the query-engine CUDA
test covers global expansion, block expansion, resident GPU query, exact
rejection of `CUDA + None`, reusable query workspace alignment, and phi-only
kernel alignment against CPU phi values.
