import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  HARDPROOF_CHECKSUMS_URL,
  HARDPROOF_INSTALL_SH_URL,
  HARDPROOF_TAG,
  hardproofAssetUrl,
} from './_config';

export default function HardproofInstall(): JSX.Element {
  const installCmd = `curl -fsSL "${HARDPROOF_INSTALL_SH_URL}" | bash -s -- --tag "${HARDPROOF_TAG}"`;
  const doctorCmd = 'hardproof doctor --machine json';
  const scanCmd =
    'hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/conformance --machine json';

  return (
    <Layout
      title="Install Hardproof"
      description="Install the Hardproof verifier binary (beta preview).">
      <main className="container margin-vert--lg">
        <h1>Install Hardproof (beta preview)</h1>
        <p>
          Prebuilt binaries for Linux and macOS. On Windows, run inside WSL2 and use the Linux
          artifact (<code>linux_x86_64</code>).
        </p>
        <p>
          Hardproof is a standalone verifier for MCP servers. You do not need to adopt x07 to use
          it. <b>Built with x07.</b>
        </p>
        <p>
          Already using the private alpha under <code>x07-mcp-test</code>? Legacy commands remain
          available during the beta transition for compatibility.
        </p>
        <p>
          Current public builds are tagged <code>{HARDPROOF_TAG}</code> while we converge on a
          stable public-beta interface.
        </p>

        <h2>Install script</h2>
        <pre>
          <code>{installCmd}</code>
        </pre>
        <p>
          The installer downloads the correct archive for your OS/arch, verifies it via{' '}
          <a href={HARDPROOF_CHECKSUMS_URL}>checksums.txt</a>, and installs into{' '}
          <code>~/.local/bin</code>.
        </p>

        <h2>Manual download</h2>
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
          Artifacts are written under <code>out/conformance/</code>:{' '}
          <code>summary.json</code>, <code>summary.junit.xml</code>, <code>summary.html</code>,{' '}
          <code>summary.sarif.json</code>.
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
