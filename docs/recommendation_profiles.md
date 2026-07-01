# Recommendation Profiles

`adasdf_recommend_build --profile path` can load a small key-value surrogate
profile. The default v1.8 profile is in:

```text
models/surrogate/v1_8/default_profile.txt
```

Supported keys:

- `memory_safety_factor`
- `error_safety_factor`
- `compression_ratio_safety_factor`
- `min_max_level`
- `max_max_level`
- `min_block_resolution`
- `max_block_resolution`
- `min_rank`
- `max_rank`
- `notes`

Unknown keys are reported as warnings and ignored. Invalid values keep the
built-in default and emit a warning.

Profiles are calibration constants only. They are not universal trained models,
not fully trained, and not optimality guarantees.
