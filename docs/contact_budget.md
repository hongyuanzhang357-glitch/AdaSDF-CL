# Contact Budget

`ContactBudget` limits the number of solver-ready candidates emitted from a
larger SDF contact candidate set.

Default values:

```text
max_contacts_total = 8
max_contacts_per_object_pair = 8
max_contacts_per_link = 4
max_contacts_per_patch = 2
```

The default stabilization path clusters candidates and uses one representative
candidate per patch before applying the total budget. This keeps the output
small and deterministic while avoiding redundant constraints in downstream
solvers.

Budgets less than or equal to zero retain no contacts and emit a warning.
Ordering is deterministic: deeper penetration first, then `sample_id`.

This is a candidate-budgeting policy, not a solver constraint generator.
