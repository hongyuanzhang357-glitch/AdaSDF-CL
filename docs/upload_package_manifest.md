# Upload Package Manifest

Use `v1.0.0-alpha` for the optional CUDA batch query benchmark pre-release.

## Include

- Source under `include`, `src`, `tools`, `tests`, and `examples`
- Small public demo surrogate metadata under `models/surrogate/demo_v0_9`
- Benchmark source and small benchmark report templates
- CMake packaging files
- Public docs
- `README.md`, `CHANGELOG.md`, `LICENSE`, and `CITATION.cff`

## Exclude

- Build trees
- Install trees
- Generated archives
- Generated `.sdfbin` files
- Generated `.svg` files
- Raw benchmark logs and large benchmark artifacts
- Raw/quarantine datasets
- Large model artifacts, joblib files, training data, and local experiment outputs

## Backend Note

v1.0 provides CPU batch query and optional CUDA batch query for analytic/demo adaptive boxes. It does not provide full low-rank compressed SDF GPU expansion, a universal trained surrogate, or full adaptive STL-to-compressed-SDF public builder.
