import {
  HARDPROOF_ACTION_USES,
  HARDPROOF_CHECKSUMS_URL,
  HARDPROOF_FEEDBACK_URL,
  HARDPROOF_INSTALL_SH_URL,
  HARDPROOF_REPO,
  HARDPROOF_TAG,
  type HardproofAssetPlatform,
  hardproofAssetName,
  hardproofAssetUrl,
  hardproofReleaseUrl,
} from '../hardproof/_config';

export const MCP_VERIFIER_REPO = HARDPROOF_REPO;
export const MCP_VERIFIER_TAG = HARDPROOF_TAG;

export const MCP_FEEDBACK_URL = HARDPROOF_FEEDBACK_URL;

export const MCP_CODESPACES_URL =
  'https://codespaces.new/x07lang/x07-mcp?quickstart=1';

export const MCP_ACTION_USES = HARDPROOF_ACTION_USES;

export {hardproofReleaseUrl as mcpReleaseUrl};
export const MCP_INSTALL_SH_URL = HARDPROOF_INSTALL_SH_URL;
export const MCP_CHECKSUMS_URL = HARDPROOF_CHECKSUMS_URL;

export type McpAssetPlatform = HardproofAssetPlatform;
export {hardproofAssetName as mcpAssetName, hardproofAssetUrl as mcpAssetUrl};
