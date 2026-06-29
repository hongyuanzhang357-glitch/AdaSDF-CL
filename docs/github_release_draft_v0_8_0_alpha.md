# AdaSDF-CL v0.8.0-alpha

First clone-only end-to-end public demo.

## Highlights

- Added a core-free analytic box SDF backend.
- Added demo `.sdfbin` text format with magic `ADASDF_DEMO_SDFBIN_V1`.
- Added `adasdf_make_demo_box`.
- `adasdf_info`, `adasdf_query`, and `adasdf_collide` can run on generated demo `.sdfbin` files without the existing research core.
- Added `adasdf_core_free_demo_collision` example.
- Updated downstream CMake example so an installed package can run a no-input demo path.
- Added tests for analytic SDF, demo `.sdfbin`, CLI generation, and core-free collision.

## Quick Start

```bash
git clone https://github.com/hongyuanzhang357-glitch/AdaSDF-CL.git
cd AdaSDF-CL
git checkout v0.8.0-alpha

cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install

install/bin/adasdf_make_demo_box cube_demo.sdfbin
install/bin/adasdf_info cube_demo.sdfbin
install/bin/adasdf_query cube_demo.sdfbin --point 0 0 0
install/bin/adasdf_collide cube_demo.sdfbin cube_demo.sdfbin --max-contacts 4
```

On Windows, use the installed `.exe` tools.

## Known Limitations

- This release adds an analytic demo SDF backend, not the full adaptive compressed SDF backend.
- The demo backend currently supports axis-aligned boxes.
- Full adaptive STL-to-sdfbin construction still requires the existing research core or future standalone builder work.
- Pair collision remains an approximate SDF-sampling narrow-phase.
- CUDA, Python bindings, and FCL ABI compatibility are not implemented.

## Validation

Expected local validation:

- CMake configure/build: PASS
- CTest: PASS
- Install validation: PASS
- Core-free make/info/query/collide demo workflow: PASS
- Repo clean check: PASS

## Upgrade Note

The earlier tags `v0.7.0-alpha`, `v0.7.0-alpha.1`, and `v0.7.0-alpha.2` are retained for traceability and should not be moved.
