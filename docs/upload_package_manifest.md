# Upload Package Manifest

Use `v0.9.0-alpha` for the surrogate-guided adaptive demo pre-release.

## Include

- Source under `include`, `src`, `tools`, `tests`, and `examples`
- Small public demo surrogate metadata under `models/surrogate/demo_v0_9`
- CMake packaging files
- Public docs
- `README.md`, `CHANGELOG.md`, `LICENSE`, and `CITATION.cff`

## Exclude

- Build trees
- Install trees
- Generated archives
- Generated `.sdfbin` files
- Generated `.svg` files
- Raw/quarantine datasets
- Large model artifacts, joblib files, training data, and local experiment outputs

## Backend Note

v0.9 provides a demo surrogate and demo adaptive builder for analytic boxes. It does not provide a universal trained surrogate or full adaptive STL-to-compressed-SDF public builder.
