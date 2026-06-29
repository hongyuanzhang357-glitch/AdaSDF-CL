# External Collision Test Summary for v0.7.0-alpha.1

## Verdict

PARTIAL

## What Passed

- Public clone
- Core-free configure/build/test/install
- Installed CLI usage smoke
- External `find_package(AdaSDFCL CONFIG REQUIRED)`
- External compile/link test
- Existing-core STL-to-sdfbin generation
- Existing-core info/query/collide
- Existing-core external collision program with dependency prefix

## What Failed or Is Blocked

- Core-free standalone `.sdfbin` generation
- Core-free standalone `.sdfbin` read/query/collide
- Existing-core install tree is not fully self-contained because downstream consumers may need dependency prefixes

## Engineering Implications

- Documentation must clearly distinguish core-free API preview from existing-core functional backend.
- A future standalone public backend or packaged legal `.sdfbin` fixture is needed for a clone-only collision demo.

## v0.8 Follow-Up

v0.8.0-alpha addresses the clone-only demo gap with an analytic box backend and demo `.sdfbin` format. The v0.7.0-alpha.1 external test remains PARTIAL for historical traceability; the recommended follow-up is a fresh external downstream test against `v0.8.0-alpha`, expected to PASS for the demo backend.
