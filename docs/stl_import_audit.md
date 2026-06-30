# STL Import Audit

v1.2.0-alpha introduces the first public STL import preflight path. The goal is
to make STL readiness visible before users attempt SDF construction.

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
| Mesh repair | Planned | - | Not implemented. |
| Self-intersection detection | Planned | - | Not implemented. |
| Full arbitrary-STL adaptive SDF builder | Planned | - | Existing-core bridge is separate. |
| FCL fallback backend | Planned | - | Not implemented. |

## Import Boundary

The v1.2 STL reader is diagnostic infrastructure. It reads triangles into a
simple public `TriangleMesh`, but it does not convert arbitrary STL files into
adaptive compressed SDF assets.

Recommended preflight:

```bash
adasdf_mesh_check robot_link.stl --out robot_link_mesh_report.md
```

If the report shows boundary edges, non-manifold edges, degenerate triangles, or
scale warnings, users should fix the source mesh or consciously accept the risk
before future SDF construction.
