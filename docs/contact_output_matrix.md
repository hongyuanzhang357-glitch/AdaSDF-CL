# Contact Output Matrix

Stable contact manifold generation is partial / research-preview. Contact
reduction exists. Full robot-grade manifold clustering and CCD remain planned.

DenseSDF models built by `adasdf_build_dense_sdf` use the same SDF-derived
contact fields as other queryable public models. Contact quality remains tied
to grid resolution, sign quality, sampling, and the research-preview contact
reduction path.

AdaptiveBlockSDF models built by `adasdf_build_adaptive_sdf` use the same
contact fields. In v1.6, adaptive blocks store dense phi values and are queried
through the public SDFModel path.

CompressedAdaptiveBlockSDF models built by `adasdf_build_compressed_sdf` or
`adasdf_compress_adaptive_sdf` also use the same contact fields. In v1.7,
matrix-SVD blocks reconstruct grid values on demand for direct CPU query, while
dense fallback blocks keep their original phi values.

v1.9 adds sparse contact candidates for sampled links, tools, feet, grippers,
and other point/sphere proxy sets. Candidate extraction is separate from full
pair collision. It keeps a deterministic Top-K set so hard-contact solvers do
not receive every penetrating sample.

v1.10 adds CPU contact-aware active block expansion/cache for the same sparse
sample and candidate workflows. It is a runtime memory strategy for compressed
SDF queries, not a full contact manifold generator.

| Contact field | Status | Source | Notes |
| --- | --- | --- | --- |
| contact point | Implemented | `Contact::point` | SDF candidate point pipeline. |
| contact normal | Implemented | `Contact::normal` | Orientation is documented but not certified. |
| penetration depth | Implemented | `Contact::penetration_depth` | SDF-derived. |
| signed distance | Implemented | `Contact::signed_distance` | Per-contact value. |
| object ids | Implemented | `object_id_a`, `object_id_b` | Defaults may be unset. |
| feature ids | Partial | `feature_id_a`, `feature_id_b` | Field exists; mesh feature mapping is incomplete. |
| gradient | Implemented | `Contact::gradient` | SDF-derived. |
| max contacts | Implemented | `CollisionRequest::max_contacts` | API and CLI enforce it. |
| raw contacts | Implemented | `CollisionResult::numRawContacts` | Before reduction. |
| reduced contacts | Implemented | `CollisionResult::numReducedContacts` | After deterministic reduction. |
| deterministic reduction | Implemented | `ContactReducer` | Research-preview reduction. |
| sparse candidate rank | Implemented | `ContactCandidate::rank` | Top-K sparse candidate output. |
| sparse effective phi | Implemented | `ContactCandidate::effective_phi` | Uses `phi - radius`. |
| sparse reduction radius | Implemented | `ContactCandidateOptions::reduction_radius` | Suppresses nearby lower-priority candidates. |
| sparse candidate budget | Implemented | `ContactCandidateOptions::top_k` | Keeps hard-contact constraint counts small. |
| nearest point A | Implemented | `DistanceResult::nearestPointA` | Distance query. |
| nearest point B | Implemented | `DistanceResult::nearestPointB` | Distance query. |
| contact manifold clustering | Partial | `ContactReducer` | Full stable manifold clustering planned. |
| contact normal orientation | Partial | SDF gradients | Needs broader validation. |
| contact confidence | Planned | - | Not implemented. |
| contact error estimate | Planned | - | Not implemented. |

Sparse contact candidates are reduced candidates, not solver contacts and not
a complete contact manifold. Solver-aware manifold generation remains planned.
