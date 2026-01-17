export const AGENT_ENDPOINTS = {
  entrypoints: '/agent/latest/entrypoints.json',
  entry_index: '/agent/latest/index.json',
  manifest: '/agent/latest/manifest.json',
  schemas_index: '/agent/latest/schemas/index.json',
  skills_index: '/agent/latest/skills/index.json',
  examples_index: '/agent/latest/examples/index.json',
  stdlib_index: '/agent/latest/stdlib/index.json',
  toolchain_versions: '/versions/toolchain_versions.json',
} as const;

export const MINIMAL_AGENT_LOOP_COMMANDS = [
  'x07 ast init --world solve-pure --module main --kind entry --out main.x07.json',
  'x07c lint --input main.x07.json --world solve-pure',
  'x07c fix --input main.x07.json --world solve-pure --write',
  'x07 test --manifest tests/tests.json',
] as const;
