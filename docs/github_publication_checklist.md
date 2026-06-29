# GitHub Publication Checklist for AdaSDF-CL v1.0.1-alpha

The original v0.7, v0.8, v0.9, and v1.0.0 tags are retained for traceability. The recommended query-mode and GPU-resident expanded SDF pre-release is `v1.0.1-alpha`.

## Repository Safety

- [x] No generated `.sdfbin` assets are committed.
- [x] No generated `.svg` assets are committed.
- [x] No build/install/dist folders are committed.
- [x] No large artifacts are committed.
- [x] No raw STL datasets are committed.
- [ ] Working tree clean after final v1.0.1 commit.

## Build and Test

- [ ] CMake configure passes locally.
- [ ] CMake build passes locally.
- [ ] CTest passes locally.
- [ ] Install validation passes locally.
- [ ] Alpha validation passes locally.
- [ ] Repo clean check passes locally.

## Publication

- [ ] Branch pushed.
- [ ] `v1.0.1-alpha` tag pushed.
- [ ] Older tags retained unchanged.
- [ ] GitHub Actions passed on GitHub.
- [ ] GitHub pre-release created from `v1.0.1-alpha`.
