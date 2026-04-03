import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  MCP_CHECKSUMS_URL,
  MCP_INSTALL_SH_URL,
  MCP_VERIFIER_TAG,
  mcpAssetUrl,
} from './_config';

export default function McpInstall(): JSX.Element {
  const installCmd = `curl -fsSL "${MCP_INSTALL_SH_URL}" | bash -s -- --tag "${MCP_VERIFIER_TAG}"`;
  const doctorCmd = 'hardproof doctor --machine json';
  const conformanceCmd =
    'hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/conformance --machine json';

  return (
    <Layout
      title="Install"
      description="Install the Hardproof verifier binary (beta preview).">
      <main className="container margin-vert--lg">
        <h1>Install Hardproof (beta preview)</h1>
        <p>
          Prebuilt binaries for Linux and macOS. On Windows, run inside WSL2 and use the Linux
          artifact.
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
          Current public builds are tagged <code>{MCP_VERIFIER_TAG}</code> while we converge on a
          stable public-beta interface.
        </p>

        <h2>Install script</h2>
        <pre>
          <code>{installCmd}</code>
        </pre>
        <p>
          The installer downloads the correct archive for your OS/arch, verifies it via{' '}
          <a href={MCP_CHECKSUMS_URL}>checksums.txt</a>, and installs into{' '}
          <code>~/.local/bin</code>.
        </p>

        <h2>Manual download</h2>
        <ul>
          <li>
            Linux x64: <a href={mcpAssetUrl('linux-x64')}>{mcpAssetUrl('linux-x64')}</a>
          </li>
          <li>
            macOS arm64: <a href={mcpAssetUrl('darwin-arm64')}>{mcpAssetUrl('darwin-arm64')}</a>
          </li>
          <li>
            macOS x64: <a href={mcpAssetUrl('darwin-x64')}>{mcpAssetUrl('darwin-x64')}</a>
          </li>
          <li>
            Checksums: <a href={MCP_CHECKSUMS_URL}>{MCP_CHECKSUMS_URL}</a>
          </li>
        </ul>

        <h2>Verify your first server</h2>
        <p>1) Run diagnostics (checks Node/npm/npx preconditions for conformance):</p>
        <pre>
          <code>{doctorCmd}</code>
        </pre>

        <p>2) Run conformance:</p>
        <pre>
          <code>{conformanceCmd}</code>
        </pre>

        <p>
          Artifacts are written under <code>out/conformance/</code>:{' '}
          <code>summary.json</code>, <code>summary.junit.xml</code>, <code>summary.html</code>,{' '}
          <code>summary.sarif.json</code>.
        </p>

        <h2>Next</h2>
        <ul>
          <li>
            Run in CI: <Link to="/mcp/action">/mcp/action</Link>
          </li>
          <li>
            Zero-install evaluation in Codespaces: <Link to="/mcp/codespaces">/mcp/codespaces</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}
