# Differentiable Contact

AdaSDF-CL should expose more than a boolean collision answer. Its SDF representation can support quantities used by simulation, optimization, and learning systems.

## Planned Outputs

- signed distance
- contact normal
- penetration depth
- SDF gradient
- optional Jacobian
- batched differentiable query
- GPU parallel query

These outputs can support contact dynamics, trajectory optimization, robot control, reinforcement learning, and differentiable simulation.

## Public Data Hook

The first-pass API reserves:

```cpp
struct DifferentialContactData
{
    Vector3 gradient_a;
    Vector3 gradient_b;
    MatrixX jacobian_a;
    MatrixX jacobian_b;

    // TODO: connect to autodiff / analytic gradient / CUDA differentiable kernels.
};
```

The current project has SDF normal and finite-difference style paths that can seed gradient work, but this first pass does not claim full automatic differentiation support.

## Implementation Plan

1. CPU signed distance query from loaded `.sdfbin`.
2. CPU gradient query using analytic reconstruction where possible and finite differences as a baseline.
3. Contact normal and penetration depth in `Contact`.
4. Batched gradient output buffers.
5. Optional Jacobian callbacks for application-owned generalized coordinates.
6. CUDA kernels for batched signed distance and gradient queries.

## Non-goals for v0.1

- No claim of complete autodiff.
- No solver-specific Jacobian convention.
- No CUDA requirement for CPU users.
- No hidden dependency on a machine learning framework.
