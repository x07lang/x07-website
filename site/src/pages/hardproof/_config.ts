export const HARDPROOF_REPO = 'x07lang/hardproof';
export const HARDPROOF_TAG = 'v0.3.0-beta.0';

export const HARDPROOF_FEEDBACK_URL = `https://github.com/${HARDPROOF_REPO}/issues/new/choose`;

export const HARDPROOF_ACTION_USES = `x07lang/hardproof/hardproof-scan@${HARDPROOF_TAG}`;

export function hardproofReleaseUrl(path: string): string {
  return `https://github.com/${HARDPROOF_REPO}/releases/download/${HARDPROOF_TAG}/${path}`;
}

export const HARDPROOF_INSTALL_SH_URL = hardproofReleaseUrl('install.sh');
export const HARDPROOF_CHECKSUMS_URL = hardproofReleaseUrl('checksums.txt');

export type HardproofAssetPlatform = 'linux_x86_64' | 'macos_arm64' | 'macos_x86_64';

export function hardproofAssetName(platform: HardproofAssetPlatform): string {
  const version = HARDPROOF_TAG.startsWith('v') ? HARDPROOF_TAG.slice(1) : HARDPROOF_TAG;
  return `hardproof_${version}_${platform}.tar.gz`;
}

export function hardproofAssetUrl(platform: HardproofAssetPlatform): string {
  return hardproofReleaseUrl(hardproofAssetName(platform));
}
