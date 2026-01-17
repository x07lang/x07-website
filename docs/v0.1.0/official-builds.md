# Official builds

An official X07 build is an artifact that:

- Is produced from an official release tag in `x07lang/x07`.
- Matches the published release manifest (when present).
- Is signed by the X07 release keys (when signing is enabled).

If a build does not meet these requirements, it must not be presented as an official build.

## Downloads

Official builds are distributed as GitHub release assets:

- https://github.com/x07lang/x07/releases

For a given release tag `vX.Y.Z`, expect:

- Toolchain builds (`x07-vX.Y.Z-*.{tar.gz,zip}`)
- Skills pack (`x07-skills-vX.Y.Z.tar.gz`)
- `release-manifest.json` (lists the expected contents and hashes)

See [Install X07](getting-started/install.md) for setup steps.
