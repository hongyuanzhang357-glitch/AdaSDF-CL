# Recommended Build Workflow

AdaSDF-CL v1.8.0-alpha adds a recommend-then-build workflow.

1. Run mesh diagnostics:

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md --json mesh_report.json
```

2. Ask for a recommendation:

```bash
adasdf_recommend_build model.stl --target-error 1e-3 --memory-mb 512 --use-case contact --out recommendation.md --json recommendation.json --emit-command
```

3. Review the recommended path, warnings, confidence, and candidate table.

4. Run the emitted build command if the recommendation matches the workflow.

5. Validate the output with:

```bash
adasdf_info model.sdfbin
adasdf_query model.sdfbin --point 0 0 0
adasdf_expansion_quality model.sdfbin --expansion global --global-resolution 32 --samples 1000 --out quality.csv
```

For compressed output, also inspect the compression and quality reports emitted
by `adasdf_build_compressed_sdf`.

The recommendation step does not build or write `.sdfbin` output. It only emits
parameters, estimates, warnings, and a command.
