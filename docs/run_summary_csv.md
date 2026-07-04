# Run Summary CSV

`adasdf_collect_run_summary` converts validated strict JSON reports into one
CSV table.

```bash
adasdf_collect_run_summary --inputs reports.txt --out run_summary.csv
```

## Inputs File

The `--inputs` file contains one strict JSON report path per line. Blank lines
are ignored.

## Output Columns

The CSV starts with all required strict report fields and then appends optional
metric columns in schema order:

```text
schema_version,adasdf_version,git_commit,tool_name,case_id,input_path,...
```

Missing optional metrics are emitted as empty cells. CSV quoting is handled by
AdaSDF-CL's local strict CSV writer.

## Failure Behavior

The collector fails if any listed report cannot be read or fails schema
validation. This keeps CI and external benchmark automation from silently
mixing incompatible report versions.
