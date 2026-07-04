# AdaSDF-CL v1.15.0-alpha

AdaSDF-CL v1.15.0-alpha adds strict JSON/CSV reporting and reproducibility
manifests for alpha build, query, benchmark, and world-collision workflows.

## Highlights

- New dependency-free report module under `include/adasdf/report`.
- Strict JSON writer, strict CSV writer, report validator, report collector,
  run manifest, and case manifest helpers.
- New CLIs:
  - `adasdf_write_manifest`
  - `adasdf_validate_report`
  - `adasdf_collect_run_summary`
- `--strict-json` and `--case-id` support on selected core CLIs.
- Python wrapper helpers for manifest writing, report validation, run summary
  collection, and strict report argument forwarding.
- Install and alpha validation now exercise strict report generation,
  validation, and summary collection.

## Boundaries

- This release does not change the core collision algorithms.
- The strict JSON report is an automation/provenance artifact, not a solver
  constraint file and not a mesh certification report.
- The report implementation does not depend on nlohmann_json, Python, FCL, or
  the existing research core.

## Suggested Checks

```bash
adasdf_build_compressed_sdf cube.stl cube.sdfbin \
  --strict-json cube.strict.json --case-id cube_build
adasdf_validate_report cube.strict.json
adasdf_collect_run_summary --inputs reports.txt --out run_summary.csv
```
