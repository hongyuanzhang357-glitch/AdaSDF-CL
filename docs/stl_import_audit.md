# STL Import Audit

v1.3.0-alpha extends the public STL import preflight path with mesh readiness
scoring and repair suggestions. The goal is to make STL suitability visible
before users attempt SDF construction.

| Capability | Status | API / Tool | Notes |
| --- | --- | --- | --- |
| ASCII STL reader | Implemented | `STLReader` | Common vertex-line STL syntax. |
| Binary STL reader | Implemented | `STLReader` | Validates triangle count and file size. |
| Duplicate vertex merge | Implemented | `STLReadOptions` | Tolerance-based quantization. |
| Triangle mesh container | Implemented | `TriangleMesh` | No Eigen dependency. |
| AABB / diagonal report | Implemented | `TriangleMesh`, `MeshDiagnostics` | Unit warning only. |
| Watertight preflight | Implemented | `MeshDiagnostics` | Topology-level check. |
| Boundary edge report | Implemented | `MeshDiagnostics` | No repair. |
| Non-manifold edge report | Implemented | `MeshDiagnostics` | No repair. |
| Degenerate triangle report | Implemented | `MeshDiagnostics` | Repeated vertices, tiny area, NaN / Inf. |
| Duplicate triangle report | Implemented | `MeshDiagnostics` | Same or reversed orientation. |
| Connected components | Implemented | `MeshDiagnostics` | Edge-adjacency components. |
| Markdown report | Implemented | `MeshDiagnosticsWriter` | Human-readable. |
| JSON-like report | Implemented | `MeshDiagnosticsWriter` | Lightweight scripting. |
| CLI preflight | Implemented | `adasdf_mesh_check` | Exit code 2 for critical mesh issues. |
| Readiness score | Implemented | `MeshReadiness` | 0-100 SDF build suitability heuristic. |
| Severity classification | Implemented | `MeshIssueSeverity` | Info, warning, and critical issues. |
| Repair suggestions | Implemented | `MeshReadinessReport` | Text guidance only; no input modification. |
| Mesh repair | Planned | - | Not implemented. |
| Self-intersection detection | Planned | - | Not implemented. |
| Full arbitrary-STL adaptive SDF builder | Planned | - | Existing-core bridge is separate. |
| FCL fallback backend | Planned | - | Not implemented. |

## Import Boundary

The STL reader and readiness scorer are diagnostic infrastructure. They read
triangles into a simple public `TriangleMesh` and classify suitability, but they
do not convert arbitrary STL files into adaptive compressed SDF assets.

Recommended preflight:

```bash
adasdf_mesh_check robot_link.stl --readiness --out robot_link_mesh_report.md
```

If the report shows boundary edges, non-manifold edges, degenerate triangles, or
scale warnings, users should fix the source mesh or consciously accept the risk
before future SDF construction. Readiness is a preflight heuristic, not an
industrial certification or automatic repair pass.
