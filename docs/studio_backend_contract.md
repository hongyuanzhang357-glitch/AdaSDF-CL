# Studio Backend Contract

Release: `v1.17.1-alpha`

Studio integrations should prefer the stable JSON CLIs:

```bash
adasdf_info model.sdfbin --json --full
adasdf_export_structure model.sdfbin --json
adasdf_export_block_grid model.sdfbin --json
adasdf_export_compression model.sdfbin --json
adasdf_collide a.sdfbin b.sdfbin --json
adasdf_benchmark_sparse_query model.sdfbin samples.csv --json
```

The JSON contract is read-only and derived from loaded runtime models. It does
not change SDFBin serialization, query algorithms, compression algorithms, or
GPU backend behavior.

Progress events are JSONL and should be read from `--progress-json` or stderr,
not stdout, when a command is also expected to produce a JSON result.
