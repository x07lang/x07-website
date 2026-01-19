# Publishing packages

X07 supports a Cargo-like workflow:

- pack
- lock
- publish

## Source-only packages

Packages publish only x07AST JSON sources.

That keeps builds:

- deterministic
- auditable
- portable across platforms

## Registry authentication

Publishing uses bearer tokens.

To publish to the official registry:

- Browse packages: https://x07.io/packages
- Index URL: `sparse+https://registry.x07.io/index/`

### Getting a token

1. Sign in with GitHub at https://x07.io/settings/tokens
2. Create a token with the `publish` scope (copy it now; you won’t be able to see it again)

Do not share tokens between people.

Publishing requires a verified email on your GitHub account.

### Storing a token locally

Prefer prompting or stdin so the token doesn’t end up in shell history:

```bash
# Prompts for token (recommended for humans)
x07 pkg login --index sparse+https://registry.x07.io/index/

# Non-interactive (recommended for CI)
printf '%s' "$X07_TOKEN" | x07 pkg login --index sparse+https://registry.x07.io/index/ --token-stdin
```

`x07 pkg login` stores credentials in `~/.x07/credentials.json` (or in `X07_PKG_HOME/credentials.json`).

### Publishing

Ensure `x07-package.json` contains a `description` (that’s what shows up on https://x07.io/packages), bump the `version`,
then publish the directory:

```bash
x07 pkg publish --index sparse+https://registry.x07.io/index/ --package .
```

### Ownership

- The first publish of a new package name creates the package and assigns you as the initial owner.
- To publish a new version of an existing package, your account must be listed as an owner.
- Owners can add other owners via the registry API (`POST https://registry.x07.io/v1/packages/<name>/owners` with JSON
  `{"handle":"<github_handle>"}`), using a bearer token with the `owner.manage` scope.

## Versioning

- package versions follow semver-style rules
- wire formats are pinned by `_v1` suffixes (see versioning policy)
