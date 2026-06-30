# Mesh Readiness

AdaSDF-CL v1.3.0-alpha adds SDF build readiness scoring on top of STL mesh
diagnostics. Readiness is a preflight heuristic. It is not an industrial
certification, automatic mesh repair, self-intersection proof, or a guarantee
that an adaptive SDF build will satisfy a downstream application.

## Purpose

Raw diagnostics answer what is present in the STL. Readiness translates those
diagnostics into:

- a 0-100 score;
- a readiness level;
- severity-ranked issues;
- recommended preprocessing steps before adaptive SDF construction.

## CLI

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_check model.stl --readiness --json mesh_report.json
adasdf_mesh_check model.stl --readiness --strict
adasdf_mesh_check model.stl --readiness --lenient
```

`--strict` requires watertight input and treats non-manifold and degenerate
geometry as critical. `--lenient` allows open meshes as warnings and lowers some
preprocessing issues from critical to warning. Neither mode modifies the input
STL.

## Score Rules

- 100: clean watertight mesh.
- 80-99: usable with minor warnings.
- 50-79: poor, needs preprocessing.
- 0-49: not recommended for SDF build.

The score is intentionally simple and explainable. Critical topology and
validity problems subtract larger penalties. Warnings such as duplicate
triangles, multiple components, isolated vertices, and unusual scale subtract
smaller penalties.

## Severity Rules

Critical issues include:

- NaN / Inf vertices or invalid geometry;
- zero triangles;
- non-manifold edges unless explicitly allowed;
- boundary edges when watertight input is required;
- degenerate triangles unless explicitly allowed;
- zero or invalid AABB diagonal.

Warnings include:

- duplicate triangles;
- multiple connected components;
- very small or very large scale;
- isolated vertices;
- boundary edges when open meshes are allowed.

Info items include:

- AABB / diagonal summary;
- component count;
- unit confirmation prompts.

## Common Suggestions

- Fill holes before building an adaptive SDF from a closed solid.
- Remove non-manifold edges.
- Remove zero-area or repeated-vertex triangles.
- Remove duplicate triangles.
- Confirm STL units and scale.
- Split unrelated connected components if they should not share one SDF asset.
- Rerun `adasdf_mesh_check --readiness` after cleanup.

## Current Limits

AdaSDF-CL v1.3.0-alpha does not automatically repair meshes. It also does not
perform full self-intersection detection and does not implement a standalone
arbitrary-STL adaptive SDF builder. The readiness report is a decision aid
before those future build paths.

## Planned Work

- Mesh repair workflows.
- Self-intersection detection.
- Standalone arbitrary STL adaptive SDF builder.
- Deeper build-time error prediction once the full public STL-to-SDF builder is
  implemented.
