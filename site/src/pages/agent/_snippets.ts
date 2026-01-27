export const AGENT_ENDPOINTS = {
  entrypoints: '/agent/latest/entrypoints.json',
  entry_index: '/agent/latest/index.json',
  manifest: '/agent/latest/manifest.json',
  schemas_index: '/agent/latest/schemas/index.json',
  skills_index: '/agent/latest/skills/index.json',
  examples_index: '/agent/latest/examples/index.json',
  packages_index: '/agent/latest/packages/index.json',
  stdlib_index: '/agent/latest/stdlib/index.json',
  toolchain_versions: '/versions/toolchain_versions.json',
} as const;

export const MINIMAL_AGENT_LOOP_COMMANDS = [
  'x07 init',
  'x07 fmt --input src/main.x07.json --write',
  'x07 lint --input src/main.x07.json',
  'x07 fix --input src/main.x07.json --write',
  'x07 run',
  'x07 test --manifest tests/tests.json',
] as const;
