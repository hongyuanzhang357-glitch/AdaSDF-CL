# Mesh Readiness

AdaSDF-CL v1.3.0-alpha adds SDF build readiness scoring on top of STL mesh
diagnostics. Readiness is a preflight heuristic. It is not an industrial
certification, automatic mesh repair, self-intersection proof, or a guarantee
that an adaptive SDF build will satisfy a downstream application.

AdaSDF-CL v1.4.0-alpha adds optional safe cleanup after readiness checks. The
cleanup path can remove duplicate/degenerate elements and write a separate STL,
but it does not fill holes, repair self-intersections, or replace a CAD repair
tool.

AdaSDF-CL v1.5.0-alpha can build a uniform DenseSDF after readiness checks.
Readiness still does not certify industrial mesh quality and does not imply the
future adaptive compressed builder is implemented.

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
adasdf_mesh_check model.stl --readiness --clean-out cleaned.stl --clean-report cleanup_report.md
```

`--strict` requires watertight input and treats non-manifold and degenerate
geometry as critical. `--lenient` allows open meshes as warnings and lowers some
preprocessing issues from critical to warning. Neither mode modifies the input
STL.

`--clean-out` writes a cleaned STL to a separate path and reruns diagnostics and
readiness on the cleaned mesh. The command refuses to overwrite the input STL.

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
- Use `adasdf_mesh_clean` or `adasdf_mesh_check --clean-out` for safe cleanup
  of duplicate/degenerate elements.
- Rerun `adasdf_mesh_check --readiness` after cleanup.

## Current Limits

AdaSDF-CL v1.5.0-alpha provides safe cleanup and a uniform DenseSDF builder,
but it does not perform hole filling, self-intersection repair, boolean
reconstruction, unit inference, or a standalone adaptive compressed SDF build.
The readiness report is a decision aid before dense builds and future adaptive
build paths.

## Planned Work

- Optional hole-filling workflows with explicit risk controls.
- Self-intersection detection.
- Standalone adaptive octree/block/low-rank SDF builder.
- Deeper build-time error prediction once the full public STL-to-SDF builder is
  implemented.
