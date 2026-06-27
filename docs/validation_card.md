# Validation Card

This page records the external DASH-SDF validation-card summary used to inform the AdaSDF-CL v0.4+ surrogate placeholder. It is a documentation import only; AdaSDF-CL does not ship the surrogate model or training artifacts.

## Source Scope

Reviewed source family:

- GeneralSTLSDFSurrogate stage-1 pilot v1.3 validation card.
- stage-1 pilot v1.3 report.
- safety calibration summary.
- global surrogate metadata summary.

No raw datasets, checkpoints, generated arrays, logs, or local absolute paths are copied into AdaSDF-CL.

## Dataset and Runs

- Approved STL dataset size: 20 models.
- Training samples: 113 total, made of 100 base samples plus 13 active-learning samples.
- Top-K real validations: 34 earlier validations plus 23 v1.3 validations.
- v1.3 active-learning build time: 3541.132 seconds.
- v1.3 training time: 2.920 seconds.
- v1.3 recommendation time: 52.447 seconds.
- v1.3 real verification time: 1888.960 seconds.
- v1.3 workspace size observed by the source project: 6.53 GB.

## Predictive Metrics

| Metric | Value |
| --- | ---: |
| p95 error MAE | 0.004969 |
| p95 error RMSE | 0.015366 |
| p95 error R2 | 0.964526 |
| memory MAE | 0.436883 |
| memory RMSE | 1.600178 |
| memory R2 | 0.999489 |
| sign mismatch accuracy | 0.929204 |
| sign mismatch precision | 0.945455 |
| sign mismatch recall | 0.912281 |

## Safety Calibration

| Field | Value |
| --- | ---: |
| target status | target_met |
| safety factor | 1.5 |
| p95 margin factor | 0.8 |
| sign-risk threshold | 0.1 |
| success-probability threshold | 0.5 |
| memory margin factor | 0.9 |
| calibrated false-safe rate | 0.0 |
| calibrated false-unsafe rate | 0.382353 |
| v1.3 false-safe rate | 0.0 |
| v1.3 false-unsafe rate | 0.130435 |

## Recommendation Success

| Metric | Value |
| --- | ---: |
| Top-1 STL-level success | 0.8 |
| Top-3 STL-level success | 1.0 |
| Top-5 STL-level success | 1.0 |
| candidate-level success | 0.826087 |

## Credibility Conclusion

The external validation card marks the surrogate as credible for its tested scope. AdaSDF-CL records that conclusion but does not treat it as a runtime guarantee.

## Limitations

- Dataset size is 20 approved STL models.
- Backend validation is targeted, not exhaustive.
- Validity depends on p95 error, memory, block-memory, and sign-mismatch constraints.
- External licensed datasets are still needed for broader evidence.
- AdaSDF-CL v0.4+ has no runtime surrogate backend, so all C++ recommendations remain non-credible placeholders.
