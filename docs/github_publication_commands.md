# GitHub Publication Commands

The original `v0.7.0-alpha` tag is retained for traceability. The recommended
first public pre-release is `v0.7.0-alpha.1`.

```bash
# 1. Verify remote:
git remote -v

# 2. Push main branch:
git push origin main

# 3. Push the recommended alpha.1 tag:
git push origin v0.7.0-alpha.1
```

Do not move or force-push the original `v0.7.0-alpha` tag. Do not create a
GitHub Release for `v0.7.0-alpha`; create the first public pre-release from
`v0.7.0-alpha.1`.
