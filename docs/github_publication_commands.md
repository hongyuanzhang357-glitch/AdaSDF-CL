# GitHub Publication Commands

The original `v0.7.0-alpha` and `v0.7.0-alpha.1` tags are retained for
traceability. The recommended documentation-hotfix pre-release is
`v0.7.0-alpha.2`.

```bash
# 1. Verify remote:
git remote -v

# 2. Push main branch:
git push origin main

# 3. Push the recommended alpha.2 tag:
git push origin v0.7.0-alpha.2
```

Do not move or force-push the original `v0.7.0-alpha` or `v0.7.0-alpha.1`
tags. Create the next GitHub pre-release from `v0.7.0-alpha.2`.
