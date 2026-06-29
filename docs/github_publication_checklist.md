# GitHub Publication Checklist for AdaSDF-CL v0.8.0-alpha

The original `v0.7.0-alpha`, `v0.7.0-alpha.1`, and `v0.7.0-alpha.2` tags are retained for traceability. The recommended clone-only demo pre-release is `v0.8.0-alpha`.

## Repository Safety

- [x] No local absolute paths found by `scripts/check_repo_clean.py`.
- [x] No large files greater than 100 MB found in the AdaSDF-CL tree.
- [x] No build/install/dist folders are committed.
- [x] No generated `.sdfbin` assets are committed.
- [x] No raw/quarantine STL datasets are committed.
- [x] No copyrighted or unclear-license assets were found in the release tree.
- [x] No experiment cache is committed.
- [x] No `.npz` arrays or large logs are committed.
- [ ] Working tree clean after final v0.8 commit.

## Build and Test

- [ ] CMake configure passes locally.
- [ ] CMake build passes locally.
- [ ] CTest passes locally.
- [ ] Install validation passes locally.
- [ ] Core-free demo make/info/query/collide workflow passes locally.
- [ ] Downstream `find_package` test passes locally.
- [ ] Alpha validation passes locally.
- [ ] Repo clean check passes locally.

## Documentation

- [x] README includes v0.8 quick start.
- [x] Core-free demo backend docs are available.
- [x] Limitations are clearly stated.
- [x] Release draft is available.
- [x] Citation file is available.
- [x] License is clear.
- [x] Contributing guide is available.
- [x] Code of conduct is available.
- [x] CI workflow is included.
- [x] Security policy is available.
- [ ] Security contact is replaced with a real maintainer contact before public release.

## Publication

- [ ] Branch pushed.
- [ ] Recommended `v0.8.0-alpha` tag pushed.
- [ ] Original `v0.7.0-alpha` tag retained unchanged.
- [ ] Original `v0.7.0-alpha.1` tag retained unchanged.
- [ ] Original `v0.7.0-alpha.2` tag retained unchanged.
- [ ] GitHub Actions passed on GitHub.
- [ ] GitHub pre-release created from `v0.8.0-alpha`.

Publication items should be checked as `main`, the `v0.8.0-alpha` tag, CI, and the GitHub pre-release are completed.
