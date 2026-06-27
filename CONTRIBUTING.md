# Contributing

Thank you for helping improve AdaSDF-CL.

## Issues

When opening an issue, include:

- operating system and compiler;
- CMake configure command;
- whether `ADASDF_CL_USE_EXISTING_CORE` was enabled;
- a minimal reproducer or command;
- expected behavior and actual behavior;
- whether CUDA, FCL, Python, or external datasets were involved.

Do not attach large generated files, raw proprietary STL datasets, build trees, `.sdfbin` assets, `.npz` arrays, or local machine paths.

## Pull Requests

Before opening a pull request:

- keep changes scoped;
- add or update tests when behavior changes;
- run CMake build and CTest where practical;
- run `python scripts/check_repo_clean.py .`;
- update docs for public API or behavior changes;
- document limitations honestly.

## Coding Style

- Prefer existing AdaSDF-CL naming and layout.
- Keep public headers dependency-light.
- Keep CUDA, FCL, Python, UI, and surrogate runtime dependencies optional.
- Use deterministic algorithms in tests and examples.
- Avoid checked-in generated artifacts.

## Assets and Data

Do not add copyrighted assets, large experimental artifacts, raw datasets, generated benchmark directories, local logs, or local absolute paths.
