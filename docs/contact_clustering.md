# Contact Clustering

`ContactClusterer` groups nearby `ContactCandidate` values into local
`ContactPatch` records.

The clustering rule is deterministic:

1. Sort candidates by penetration depth descending, then `sample_id`.
2. Assign each candidate to the first compatible nearby patch.
3. Compatibility requires distance to patch centroid within `spatial_radius`.
4. When normal consistency is enabled, the candidate normal must agree with the
   patch average normal by cosine threshold.
5. Otherwise create a new patch, up to `max_patches`.

Each patch reports centroid, average normal, max/mean penetration, members, and
a representative sample id. The representative is the deepest candidate, with
`sample_id` as the deterministic tie-breaker.

Clustering reduces redundant nearby samples. It does not create contact
manifold constraints or friction directions.
