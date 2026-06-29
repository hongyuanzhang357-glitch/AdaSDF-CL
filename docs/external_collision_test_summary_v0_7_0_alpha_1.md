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
