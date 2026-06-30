# STL Mesh Diagnostics

AdaSDF-CL v1.2.0-alpha adds STL mesh diagnostics as a preflight step before
future SDF construction. It helps users inspect whether an STL is a reasonable
candidate for adaptive SDF building, FCL fallback experiments, or a future
CollisionWorld pipeline.

This is not a mesh repair tool and not a full standalone arbitrary-STL adaptive
SDF builder.

## Supported Input

- ASCII STL.
- Binary STL.

Binary STL loading validates the triangle count against file size. ASCII STL
loading handles common `facet` / `outer loop` / `vertex` syntax.

## Detected Items

- Vertex count.
- Triangle count.
- Raw triangle count.
- AABB min/max.
- AABB diagonal length.
- NaN / Inf or invalid index presence.
- Degenerate triangles.
- Duplicate triangles.
- Boundary edges.
- Non-manifold edges.
- Connected components.
- Watertight status.
- Isolated vertices.
- Very small / very large scale warnings.
- Recommendation text.

## Definitions

### Watertight

The current watertight check is topology-level:

```text
boundary_edge_count == 0
non_manifold_edge_count == 0
triangle_count > 0
```

This does not prove the mesh is free from self-intersections.

### Boundary Edge

A boundary edge is an undirected edge used by exactly one triangle.

### Non-Manifold Edge

A non-manifold edge is an undirected edge used by three or more triangles.

### Degenerate Triangle

A triangle is degenerate if:

- two or three vertex indices are identical;
- area is below `degenerate_area_epsilon`;
- any referenced vertex contains NaN or Inf.

### Duplicate Triangle

A duplicate triangle has the same three vertex positions as an earlier triangle
within the configured tolerance. Same and reversed orientation are both counted
as duplicate candidates.

### Scale Warning

AdaSDF-CL does not infer units. It only warns when the AABB diagonal is very
small (`< 1e-6`) or very large (`> 1e6`) and asks users to confirm units.

## CLI

```bash
adasdf_mesh_check model.stl
adasdf_mesh_check model.stl --out mesh_report.md
adasdf_mesh_check model.stl --json mesh_report.json
adasdf_mesh_check model.stl --tolerance 1e-12 --area-eps 1e-14
```

Options:

- `--out report.md`: write Markdown.
- `--json report.json`: write JSON-like text.
- `--tolerance value`: vertex merge and duplicate-triangle tolerance.
- `--area-eps value`: degenerate triangle area threshold.
- `--no-duplicate-check`: skip duplicate-triangle detection.
- `--no-components`: skip connected-component detection.
- `--verbose`: print warnings and errors.

## Return Codes

- `0`: STL read and diagnostics completed without critical issues.
- `1`: STL read failed or command-line parsing failed.
- `2`: diagnostics completed, but the mesh has critical issues such as boundary
  edges, non-manifold edges, or degenerate triangles.

## Report Fields

Markdown reports contain:

- Summary.
- AABB / scale.
- Counts.
- Watertight status.
- Problems.
- Sample bad elements.
- Recommendation.

JSON-like reports expose the same core fields for lightweight scripting without
adding a third-party JSON dependency.

## Current Limits

- No automatic mesh repair.
- No self-intersection detection.
- No unit inference.
- No full arbitrary-STL adaptive SDF builder.
- No FCL fallback backend.
- No ROS / MoveIt integration.

## Planned Work

- Mesh repair recommendations and optional repair passes.
- Self-intersection detection.
- Full standalone arbitrary-STL adaptive SDF builder.
- Real robot STL benchmark suite.
