# AdaSDF-CL v1.8 Surrogate Profile

This directory contains a small deterministic key-value profile used by the
v1.8.0-alpha build recommender.

It is not a universal trained model, not fully trained, and not an optimality
guarantee. The constants only calibrate conservative memory, error, compression
ratio, level, block-resolution, and rank heuristics for the core-free public
recommendation path.

Use `adasdf_recommend_build --profile default_profile.txt` to override the
built-in defaults with this file or a compatible local profile.
