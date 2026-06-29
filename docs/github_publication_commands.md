# GitHub Publication Commands

Do not run these commands until the GitHub repository URL has been confirmed.
No remote has been added by the preflight process.

```bash
# 1. Create an empty GitHub repository first, then set the remote:
git remote add origin <YOUR_GITHUB_REMOTE_URL>

# 2. Verify remote:
git remote -v

# 3. Push main branch:
git branch -M main
git push -u origin main

# 4. Push alpha tag:
git push origin v0.7.0-alpha
```

Current local branch before publication is expected to be `master`. The command
above renames it to `main` immediately before the first push.
