# Collision Visualization

AdaSDF-CL v0.9.0-alpha includes a dependency-free SVG collision writer.

## Scope

`CollisionSVGWriter` writes a simple XY projection. It does not depend on Qt, OpenGL, Python, matplotlib, or a browser runtime.

The SVG contains:

- box A outline;
- box B outline;
- contact markers;
- normal arrows;
- title;
- minimum distance;
- requested max contacts;
- returned contacts;
- backend and method labels.

## CLI

```bash
adasdf_collide_boxes_demo --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view collision.svg
```

Generated SVG files are build artifacts and should not be committed to the source tree.
