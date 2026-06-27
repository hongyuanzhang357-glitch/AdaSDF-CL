# AdaSDF-CL Python Plan

The Python package is intentionally not implemented in the first pass. The planned binding layer should wrap the stable C++ API after CPU `.sdfbin` loading and query paths are connected.

Planned modules:

- `adasdf.io`: load and save `.sdfbin` assets.
- `adasdf.geometry`: `SDFModel`, `CollisionObject`, and transforms.
- `adasdf.query`: collision, distance, contact, and batch APIs.
- `adasdf.cuda`: optional GPU batch-query helpers.

Binding notes:

- Prefer NumPy-compatible buffers for batched query points and contacts.
- Do not require CUDA to import the CPU package.
- Keep file-format compatibility tests shared with C++.
