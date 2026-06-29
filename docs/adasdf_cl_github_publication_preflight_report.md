# AdaSDF-CL GitHub Publication Preflight Report

## Goal

Prepare AdaSDF-CL `v0.7.0-alpha` as the first public GitHub alpha release
candidate without adding a remote, pushing, changing the version number, or
introducing new core algorithms.

## Completed Work

- Audited public-facing README, changelog, citation, license, contribution,
  conduct, status, limitation, release-note, external-integration, CI, and issue
  template files.
- Added publication checklist, GitHub release draft, manual publication command
  notes, and security policy.
- Strengthened repository hygiene checks for package archives and stale remote
  placeholders.
- Updated public README and external-integration examples to prefer build/install
  directories outside the source tree.
- Re-ran publication build, test, install, package, alpha, and clean validation.
- Prepared the repository for a local annotated `v0.7.0-alpha` tag after this
  report is committed.

## New Files

- `SECURITY.md`
- `docs/github_publication_checklist.md`
- `docs/github_publication_commands.md`
- `docs/github_release_draft_v0_7_0_alpha.md`
- `docs/adasdf_cl_github_publication_preflight_report.md`

## Modified Files

- `.gitignore`
- `README.md`
- `docs/external_integration.md`
- `reports/alpha_validation_summary.md`
- `reports/install_validation_summary.md`
- `scripts/check_repo_clean.py`

## Version

- Current public version: `0.7.0-alpha`.
- Version number was not changed in this preflight round.
- MIT license retained.

## Git State

- Base commit at the start of preflight: `5399874`.
- Branch at preflight time: `master`.
- Final preflight commit hash: recorded in the final handoff after this report is
  committed. A tracked file cannot embed the final hash of the commit that
  contains it without changing that hash.
- Tag before preflight commit: `v0.7.0-alpha` did not exist.
- Local annotated tag plan: create `v0.7.0-alpha` after the preflight commit so
  it points at the latest publication-ready commit.
- Remote: none configured.
- Push performed: no.

## Public README Audit

Result: PASS.

- No local absolute paths found.
- Public status is consistently `0.7.0-alpha / research preview`.
- Limitations are stated clearly.
- README does not claim industrial certification.
- README does not claim FCL ABI compatibility or drop-in replacement status.
- README states DASH-SDF surrogate support is placeholder-only unless a future
  backend is connected.
- Example build paths were adjusted to prefer source-tree-external build
  directories.

## Release Draft

Result: READY AS DRAFT.

- Draft file: `docs/github_release_draft_v0_7_0_alpha.md`.
- It is suitable for a GitHub Release page after the repository is pushed.
- It states highlights, validation, known limitations, recommended use, and
  non-recommended use.

## Publication Checklist

Result: READY WITH INTENTIONAL UNCHECKED ITEMS.

- Checklist file: `docs/github_publication_checklist.md`.
- Repository safety, build/test, and documentation items are checked where
  locally verified.
- GitHub publication items remain unchecked until the user confirms a repository
  URL and requests push/release actions.
- Security contact remains a pre-public-release follow-up.

## Security File

Result: PRESENT.

- File: `SECURITY.md`.
- Security contact is explicitly marked as: `to be added before public release`.
- It asks users not to post sensitive or closed-source models in public issues.
- It states the project is alpha/research-preview and not industrial
  safety-certified.

## CI Static Review

Result: PASS.

- CI does not use local absolute paths.
- CI configures with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- CI does not require CUDA.
- CI does not require FCL.
- CI does not require Python package bindings.
- CI runs on Windows and Ubuntu.
- CI builds outside the checkout tree, runs CTest, runs install validation, and
  runs repository clean check.
- GitHub Actions were not run locally; this is a static workflow review plus
  local command-equivalent validation.

## Local Validation

Publication configure:

```bash
cmake -S . -B ../build/adasdf_cl-publication-check -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON
```

Result: PASS.

Publication build:

```bash
cmake --build ../build/adasdf_cl-publication-check --config Debug
```

Result: PASS.

CTest:

```bash
ctest --test-dir ../build/adasdf_cl-publication-check -C Debug --output-on-failure
```

Result: PASS, 17/17 tests.

Install:

```bash
cmake --install ../build/adasdf_cl-publication-check --config Debug --prefix ../build/adasdf_cl-publication-install
```

Result: PASS.

Install validation:

```bash
python scripts/run_install_validation.py --source . --build ../build/adasdf_cl-publication-package-test --install ../build/adasdf_cl-publication-install --config Debug
```

Result: PASS.

Alpha validation:

```bash
python scripts/run_alpha_validation.py --source . --build ../build/adasdf_cl-publication-alpha --config Debug --include-install
```

Result: PASS.

Repository clean check:

```bash
python scripts/check_repo_clean.py .
```

Result: PASS.

## Risk Audit

- Local absolute paths found in committed text: no.
- Large files greater than 100 MB found in the AdaSDF-CL tree: no.
- Generated `.sdfbin`, `.npz`, logs, package archives, build/install/dist trees
  in the AdaSDF-CL tree: no.
- Unclear-license assets found: no.
- Fake DOI found: no.
- Overstated industrial/FCL/surrogate claims found: no.
- Stale remote placeholder found outside the approved publication command/report
  files: no.

## Local Tag

The local annotated tag should be created after this report is committed:

```bash
git tag -a v0.7.0-alpha -m "AdaSDF-CL v0.7.0-alpha research preview"
```

No tag push should be performed until the user confirms the GitHub repository
URL and explicitly requests publication.

## Manual GitHub Publication Steps

After the user creates an empty GitHub repository and confirms the URL:

```bash
git remote add origin <YOUR_GITHUB_REMOTE_URL>
git remote -v
git branch -M main
git push -u origin main
git push origin v0.7.0-alpha
```

Then create a GitHub Release using
`docs/github_release_draft_v0_7_0_alpha.md`.

## Current Limitations

- Pair collision remains an approximate CPU SDF-sampling narrow-phase.
- FCL ABI compatibility is not implemented.
- CUDA batched pair query is not implemented.
- Python package bindings are not implemented.
- The true DASH-SDF surrogate backend is not connected.
- Contact-only `.sdfbin` writer/reader is not implemented.
- AdaSDF-CL is not an industrial-certified collision pipeline.

## Next Recommendations

1. Add a real security contact before public release if possible.
2. Confirm the GitHub repository URL.
3. Push branch and `v0.7.0-alpha` tag only after user approval.
4. Confirm GitHub Actions pass on the hosted repository.
5. Create the GitHub Release using the prepared release draft.
6. If publication is deferred, proceed to the v0.8 Python binding preview.
