# Contact Output Matrix

Stable contact manifold generation is partial / research-preview. Contact
reduction exists. Full robot-grade manifold clustering and CCD remain planned.

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
| nearest point A | Implemented | `DistanceResult::nearestPointA` | Distance query. |
| nearest point B | Implemented | `DistanceResult::nearestPointB` | Distance query. |
| contact manifold clustering | Partial | `ContactReducer` | Full stable manifold clustering planned. |
| contact normal orientation | Partial | SDF gradients | Needs broader validation. |
| contact confidence | Planned | - | Not implemented. |
| contact error estimate | Planned | - | Not implemented. |
