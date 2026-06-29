# Demo v0.9 Surrogate Metadata

This directory contains only public, lightweight metadata for the AdaSDF-CL v0.9 demo surrogate recommender.

It does not contain a trained model artifact, joblib file, raw STL data, build results, local paths, or private datasets.

The C++ recommender is a deterministic demo rule table used to exercise the public workflow:

```text
target error + memory constraints
-> demo parameter recommendation
-> demo adaptive SDF metadata
-> query/collision/SVG visualization
```

The recommender is experimental demo infrastructure. It is not universal, not fully trained, and not an optimality guarantee.
