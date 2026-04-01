export const MCP_ALPHA_VERIFIER_REPO = 'x07lang/x07-mcp-test';
export const MCP_ALPHA_VERIFIER_TAG = 'v0.1.0-alpha.4';

export const MCP_ALPHA_FEEDBACK_URL = `https://github.com/${MCP_ALPHA_VERIFIER_REPO}/issues/new/choose`;

export const MCP_ALPHA_CODESPACES_URL =
  'https://codespaces.new/x07lang/x07-mcp?quickstart=1';

export const MCP_ALPHA_ACTION_USES = `x07lang/x07-mcp-test/action@${MCP_ALPHA_VERIFIER_TAG}`;

export function mcpAlphaReleaseUrl(path: string): string {
  return `https://github.com/${MCP_ALPHA_VERIFIER_REPO}/releases/download/${MCP_ALPHA_VERIFIER_TAG}/${path}`;
}

export const MCP_ALPHA_INSTALL_SH_URL = mcpAlphaReleaseUrl('install.sh');
export const MCP_ALPHA_CHECKSUMS_URL = mcpAlphaReleaseUrl('checksums.txt');

export type McpAlphaAssetPlatform = 'linux-x64' | 'darwin-arm64' | 'darwin-x64';

export function mcpAlphaAssetName(platform: McpAlphaAssetPlatform): string {
  return `x07-mcp-test-${MCP_ALPHA_VERIFIER_TAG}-${platform}.tar.gz`;
}

export function mcpAlphaAssetUrl(platform: McpAlphaAssetPlatform): string {
  return mcpAlphaReleaseUrl(mcpAlphaAssetName(platform));
}
