# Contact-Band Marker Cache

Release: `v1.17.2-alpha`

Contact-band construction can spend significant time testing marker cells and
refining box-triangle distances. v1.17.2-alpha adds a CPU-only marker cache for
build-time reuse.

## What Is Cached

- marker cell decisions;
- candidate triangle id lists;
- saved box-triangle distance work counters.

The cache key includes block id, marker cell coordinates, level, and a marker
configuration id. The configuration id is derived from contact-band width,
marker mode, marker cell size factor, safety factor, band limits, and halo
settings. If marker parameters change, cached decisions are not reused.

## Boundaries

- The cache does not change contact-band sampling policy.
- The cache does not change `.sdfbin` output format.
- The cache is disabled by `--marker-cache off`.
- Marker cache hit counts are reported through build cache stats and build
  profile counters.

The current implementation is conservative and local to the build path. Future
work can broaden candidate cache reuse where the key can prove parameter and
geometry equivalence.
