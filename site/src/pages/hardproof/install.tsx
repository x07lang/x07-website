import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  HARDPROOF_CHECKSUMS_URL,
  HARDPROOF_INSTALL_SH_MAIN_URL,
  HARDPROOF_INSTALL_SH_URL,
  HARDPROOF_TAG,
  hardproofAssetUrl,
} from './_config';

export default function HardproofInstall(): ReactNode {
  const installLatestBetaCmd = `curl -fsSL "${HARDPROOF_INSTALL_SH_MAIN_URL}" | bash -s -- --tag latest-beta`;
  const installPinnedCmd = `curl -fsSL "${HARDPROOF_INSTALL_SH_URL}" | bash -s -- --tag "${HARDPROOF_TAG}"`;
  const doctorCmd = 'hardproof doctor --machine json';
  const scanCmd = 'hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/scan';

  return (
    <Layout
      title="Install Hardproof"
      description="Install the standalone Hardproof verifier binary.">
      <main className="container margin-vert--lg">
        <h1>Install Hardproof</h1>
        <p>
          Prebuilt binaries for Linux and macOS. On Windows, run inside WSL2 and use the Linux
          artifact (<code>linux_x86_64</code>).
        </p>
        <p>
          Hardproof is a standalone verifier for MCP servers. You do not need to adopt x07 to use
          it. <b>Built with x07.</b>
        </p>
        <p>
          Pinned beta tag (reproducible): <code>{HARDPROOF_TAG}</code>.
        </p>

        <h2>Install script</h2>
        <p>Install the latest beta (recommended):</p>
        <pre>
          <code>{installLatestBetaCmd}</code>
        </pre>
        <p>Install a pinned beta tag (reproducible):</p>
        <pre>
          <code>{installPinnedCmd}</code>
        </pre>
        <p>
          The installer downloads the correct archive for your OS/arch, verifies it via the
          release <code>checksums.txt</code>, and installs into <code>~/.local/bin</code>.
        </p>

        <h2>Manual download (pinned tag)</h2>
        <ul>
          <li>
            Linux x86_64:{' '}
            <a href={hardproofAssetUrl('linux_x86_64')}>{hardproofAssetUrl('linux_x86_64')}</a>
          </li>
          <li>
            macOS arm64:{' '}
            <a href={hardproofAssetUrl('macos_arm64')}>{hardproofAssetUrl('macos_arm64')}</a>
          </li>
          <li>
            macOS x86_64:{' '}
            <a href={hardproofAssetUrl('macos_x86_64')}>{hardproofAssetUrl('macos_x86_64')}</a>
          </li>
          <li>
            Checksums: <a href={HARDPROOF_CHECKSUMS_URL}>{HARDPROOF_CHECKSUMS_URL}</a>
          </li>
        </ul>

        <h2>Verify your first server</h2>
        <p>1) Run diagnostics (checks your environment and target reachability):</p>
        <pre>
          <code>{doctorCmd}</code>
        </pre>

        <p>2) Run a scan:</p>
        <pre>
          <code>{scanCmd}</code>
        </pre>

        <p>
          Artifacts are written under <code>out/scan/</code>: <code>scan.json</code> and{' '}
          <code>scan.events.jsonl</code>.
        </p>

        <h2>Next</h2>
        <ul>
          <li>
            Use Hardproof in CI: <Link to="/hardproof/ci">/hardproof/ci</Link>
          </li>
          <li>
            FAQ / migration: <Link to="/hardproof/faq">/hardproof/faq</Link>
          </li>
          <li>
            Build MCP servers in x07 (Codespaces): <Link to="/mcp/codespaces">/mcp/codespaces</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}
