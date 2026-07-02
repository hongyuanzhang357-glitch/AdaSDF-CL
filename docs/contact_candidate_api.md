# Contact Candidate API

AdaSDF-CL v1.9.0-alpha introduces contact candidate extraction on top of sparse
SDF query results. Candidates are intended as a compact input to downstream
contact handling. They are not full solver contacts and they are not a complete
contact manifold.

## Reduction Rule

`ContactCandidateReducer` uses this deterministic order:

1. Filter samples with `effective_phi <= candidate_threshold`.
2. Sort by `effective_phi` from small to large, so deeper penetration comes
   first.
3. Break ties by `sample_id`.
4. If `reduction_radius > 0`, suppress later candidates that are spatially
   close to an already selected candidate.
5. Keep at most `top_k` candidates.

The current penetration depth is:

```text
penetration_depth = max(0, candidate_threshold - effective_phi)
```

## CLI

```bash
adasdf_contact_candidates model.sdfbin samples.csv \
  --top-k 8 \
  --threshold 1e-3 \
  --reduction-radius 0.02 \
  --with-normal \
  --out candidates.csv
```

The candidate CLI computes normals by default because contact candidates are
commonly consumed by contact pipelines. Use `--no-normal` for phi-only candidate
ranking.

## Output Fields

Candidate CSV output includes:

```text
rank,sample_id,x,y,z,radius,phi,effective_phi,penetration_depth,
normal_x,normal_y,normal_z,object_id,link_id,group_id,label
```

## Solver Boundary

Hard-contact solvers are sensitive to constraint count. v1.9 therefore focuses
on Top-K selection and deterministic reduction instead of passing every
penetrating sample to a solver.

In v1.10, those sparse samples or reduced candidates can also drive CPU active
block selection through `ActiveBlockSelector`, allowing compressed SDF workflows
to expand only local blocks before repeated queries.
