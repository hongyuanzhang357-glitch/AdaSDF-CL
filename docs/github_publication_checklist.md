# GitHub Publication Checklist for AdaSDF-CL v0.7.0-alpha.2

The original `v0.7.0-alpha` and `v0.7.0-alpha.1` tags are retained for
traceability. The recommended documentation-hotfix pre-release is
`v0.7.0-alpha.2`.

## Repository Safety

- [x] No local absolute paths found by `scripts/check_repo_clean.py`.
- [x] No large files greater than 100 MB found in the AdaSDF-CL tree.
- [x] No build/install/dist folders are committed.
- [x] No generated `.sdfbin` assets are committed.
- [x] No raw/quarantine STL datasets are committed.
- [x] No copyrighted or unclear-license assets were found in the release tree.
- [x] No experiment cache is committed.
- [x] No `.npz` arrays or large logs are committed.
- [x] Working tree clean before preflight edits.
- [ ] Working tree clean after final publication-preflight commit.

## Build and Test

- [x] CMake configure passes locally.
- [x] CMake build passes locally.
- [x] CTest passes locally.
- [x] Install validation passes locally.
- [x] Downstream `find_package` test passes locally.
- [x] Alpha validation passes locally.
- [x] Repo clean check passes locally.

## Documentation

- [x] README is public-facing.
- [x] Limitations are clearly stated.
- [x] Release notes are available.
- [x] Citation file is available.
- [x] License is clear.
- [x] Contributing guide is available.
- [x] Code of conduct is available.
- [x] CI workflow is included.
- [x] Security policy is available.
- [ ] Security contact is replaced with a real maintainer contact before public release.

## Publication

- [ ] GitHub repository created.
- [ ] Remote URL confirmed.
- [ ] Remote added.
- [ ] Branch pushed.
- [ ] Recommended `v0.7.0-alpha.2` tag pushed.
- [ ] Original `v0.7.0-alpha` tag retained unchanged.
- [ ] Original `v0.7.0-alpha.1` tag retained unchanged.
- [ ] GitHub Actions passed on GitHub.
- [ ] GitHub pre-release created from `v0.7.0-alpha.2`.

Publication items should be checked as `main`, the `v0.7.0-alpha.2` tag, CI, and
the GitHub pre-release are completed.
