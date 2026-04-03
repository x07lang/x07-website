export const MCP_VERIFIER_REPO = 'x07lang/hardproof';
export const MCP_VERIFIER_TAG = 'v0.1.0-alpha.6';

export const MCP_FEEDBACK_URL = `https://github.com/${MCP_VERIFIER_REPO}/issues/new/choose`;

export const MCP_CODESPACES_URL =
  'https://codespaces.new/x07lang/x07-mcp?quickstart=1';

export const MCP_ACTION_USES = `x07lang/hardproof/hardproof-scan@${MCP_VERIFIER_TAG}`;

export function mcpReleaseUrl(path: string): string {
  return `https://github.com/${MCP_VERIFIER_REPO}/releases/download/${MCP_VERIFIER_TAG}/${path}`;
}

export const MCP_INSTALL_SH_URL = mcpReleaseUrl('install.sh');
export const MCP_CHECKSUMS_URL = mcpReleaseUrl('checksums.txt');

export type McpAssetPlatform = 'linux_x86_64' | 'macos_arm64' | 'macos_x86_64';

export function mcpAssetName(platform: McpAssetPlatform): string {
  const version = MCP_VERIFIER_TAG.startsWith('v') ? MCP_VERIFIER_TAG.slice(1) : MCP_VERIFIER_TAG;
  return `hardproof_${version}_${platform}.tar.gz`;
}

export function mcpAssetUrl(platform: McpAssetPlatform): string {
  return mcpReleaseUrl(mcpAssetName(platform));
}
