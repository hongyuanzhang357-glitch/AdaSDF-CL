# Existing-Core Expansion Bridge

v1.1 adds an explicit sampled bridge from existing-core `.sdfbin` models to
ExpandedSDF layouts:

```text
SDFBinReader::read(model.sdfbin)
-> SDFModel::sampleDistance()
-> SDFExpander::expand()
-> ExpandedSDF
-> ExpansionQuality::compareAgainstDirect()
-> CPU/CUDA expanded query or benchmark
```

If the existing-core model has a finite AABB and direct query support, global
expansion can sample it. If block metadata is available, block expansion uses
that metadata. If block metadata is not available, block expansion falls back to
a single synthetic block over the model AABB.

## CI Boundary

The public CI does not require the existing research core. Tests that need a
real existing-core fixture gracefully skip when the core or fixture is not
available. The core-free demo adaptive backend exercises the same sampled
ExpandedSDF bridge in every CPU-only build.

## Not Yet Implemented

This bridge samples existing-core models into expanded dense layouts. It does
not implement native low-rank compressed GPU query and does not upload
compressed low-rank factors directly to CUDA kernels.
