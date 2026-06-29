# GitHub Publication Commands

The original `v0.7.0-alpha`, `v0.7.0-alpha.1`, and `v0.7.0-alpha.2` tags are retained for traceability. The recommended clone-only demo pre-release is `v0.8.0-alpha`.

```bash
# 1. Verify remote:
git remote -v

# 2. Push main branch:
git push origin main

# 3. Push the recommended v0.8 tag:
git push origin v0.8.0-alpha
```

Do not move or force-push the original v0.7 tags. Create the next GitHub pre-release from `v0.8.0-alpha`.
