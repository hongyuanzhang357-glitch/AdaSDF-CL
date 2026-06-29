# AdaSDF-CL v0.7.0-alpha.2

Documentation hotfix after external downstream collision testing.

This pre-release clarifies the current backend boundary of AdaSDF-CL.

## What Works in the Public Core-Free Build

- Configure / build / test / install
- CMake package export
- `find_package(AdaSDFCL CONFIG REQUIRED)`
- External compile/link integration
- CLI tool installation
- API preview for SDF-based collision workflows

## What Requires the Existing Research Core

- Building real `.sdfbin` files from STL
- Reading real existing-core `.sdfbin` files
- Point signed-distance and gradient query on real `.sdfbin`
- CPU SDF-sampling pair collision on real `.sdfbin`

## External Test Verdict

`PARTIAL`

The public core-free package is usable as a CMake-integrated API preview, but a clone-only end-to-end public collision demo is not yet available.

## Known Limitations

- Core-free standalone `.sdfbin` backend is not implemented
- Pair collision remains an approximate CPU SDF-sampling narrow-phase
- Not FCL ABI-compatible
- CUDA batched pair query is not implemented
- Python package is not implemented
- True surrogate backend is not connected
- Contact-only sdfbin writer/reader is not implemented
- Not an industrial-certified collision pipeline
