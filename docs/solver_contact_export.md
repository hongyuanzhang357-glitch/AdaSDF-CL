# Solver Contact Export

`SolverContact` is a lightweight solver-input record derived from stabilized
SDF contact candidates.

Fields include:

- `contact_id`
- `sample_id`
- `patch_id`
- point and normal
- `penetration_depth`
- `phi`
- `effective_phi`
- object/link/group ids
- `label`
- `stable_key`

The stable key format is:

```text
object_id:link_id:group_id:patch_id:sample_id
```

The exporter supports CSV, JSON-like text, and Markdown. CSV columns are:

```text
contact_id,sample_id,patch_id,x,y,z,nx,ny,nz,penetration_depth,phi,effective_phi,object_id,link_id,group_id,stable_key,label
```

These exports are solver-ready candidates, not solver constraints. They do not
include impulses, friction, Jacobians, warm-start impulses, or solver residuals.
