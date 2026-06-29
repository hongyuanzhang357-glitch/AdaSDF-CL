# Query Modes And Expansion Modes

AdaSDF-CL v1.0.1-alpha separates query execution into two decisions:

- backend: where the query runs;
- expansion: what representation is queried.

This makes the CUDA boundary explicit while preserving the CPU direct path as
the default.

## Configuration

```cpp
adasdf::QueryModeConfig config;
config.backend = adasdf::QueryBackend::CPU;
config.expansion = adasdf::QueryExpansionMode::None;
config.block_selection = adasdf::BlockSelection::all();
config.allow_fallback_to_cpu = true;
config.keep_expanded_data_resident = true;
```

Factory helpers are available:

```cpp
adasdf::QueryModeConfig::cpuDirect();
adasdf::QueryModeConfig::cpuGlobalExpanded();
adasdf::QueryModeConfig::cpuBlockExpanded(adasdf::BlockSelection::all());
adasdf::QueryModeConfig::cudaGlobalExpanded();
adasdf::QueryModeConfig::cudaBlockExpanded(adasdf::BlockSelection::selected({0, 1, 2}));
```

## Supported Matrix

| Backend | Expansion | Execution | Status |
| --- | --- | --- | --- |
| CPU | None | Direct calls into `SDFModel` | Supported |
| CPU | Global | `ExpandedSDF` global dense grid | Supported |
| CPU | Block | `ExpandedSDF` selected dense blocks | Supported |
| CUDA | Global | Resident GPU global dense grid | Supported when CUDA is enabled |
| CUDA | Block | Resident GPU selected dense blocks | Supported when CUDA is enabled |
| CUDA | None | Compressed-direct GPU query | Unsupported |

Invalid CUDA direct configuration reports:

```text
CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.
```

## Expansion Options

```cpp
adasdf::ExpansionOptions expansion;
expansion.expansion = adasdf::QueryExpansionMode::Block;
expansion.block_selection = adasdf::BlockSelection::selected({0, 1, 2});
expansion.global_resolution = 64;
expansion.block_resolution = 32;
expansion.padding = 0.0;
```

`Global` expansion creates one dense grid over the model bounding box. `Block`
expansion creates one dense grid per selected block. `BlockSelection::all()`
selects every block in the model metadata. `BlockSelection::selected(...)`
sorts and deduplicates the requested block IDs and rejects negative IDs.

If a model has no block metadata, block expansion uses one synthetic block over
the model bounding box. If block metadata exists and a requested block ID is not
present, expansion fails.

## QueryEngine

```cpp
adasdf::QueryEngine engine(model, config, expansion);

if (!engine.prepare()) {
  throw std::runtime_error("query engine preparation failed");
}

adasdf::BatchQueryOutput out = engine.queryBatch(points);
const adasdf::QueryEngineStats& stats = engine.stats();
```

`prepare()` performs expansion and, for CUDA modes, resident GPU upload. CPU
direct mode does not allocate expanded data.

Statistics include:

- selected backend and expansion;
- execution mode;
- expanded host memory bytes;
- resident GPU memory bytes;
- setup time;
- CUDA kernel time when available;
- total query time;
- fallback count.

## CLI

Smoke-test query modes:

```bash
adasdf_query_mode_demo --backend cpu --expansion none --points 100000
adasdf_query_mode_demo --backend cpu --expansion global --points 100000
adasdf_query_mode_demo --backend cuda --expansion global --points 100000
adasdf_query_mode_demo --backend cuda --expansion block --blocks 0,1,2 --points 100000
```

Collision CLI request wiring:

```bash
adasdf_collide model_a.sdfbin model_b.sdfbin --backend cpu --expansion none
adasdf_collide model_a.sdfbin model_b.sdfbin --backend cuda --expansion global
adasdf_collide model_a.sdfbin model_b.sdfbin --backend cuda --expansion block --blocks 0,1,2
```

The collision API still uses the existing CPU SDF-sampling narrow-phase after
query-mode validation. The CUDA collision request path exists so public callers
can express intent and receive honest validation, but v1.0.1-alpha does not add
a GPU pair-collision solver.

## Accuracy Boundary

CPU direct queries use the model's native SDF sampler. Expanded modes query
dense samples through trilinear interpolation. Distance and normal values can
therefore differ from CPU direct values, especially near cell boundaries or
where finite-difference normals cross discontinuities.

Use expanded modes to validate query plumbing and GPU-resident throughput. Do
not treat v1.0.1-alpha expanded-grid normals as certified contact normals.
