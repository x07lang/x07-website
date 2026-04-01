import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  MCP_ALPHA_CHECKSUMS_URL,
  MCP_ALPHA_INSTALL_SH_URL,
  MCP_ALPHA_VERIFIER_TAG,
  mcpAlphaAssetUrl,
} from './_config';

export default function McpInstall(): JSX.Element {
  const installCmd = `curl -fsSL "${MCP_ALPHA_INSTALL_SH_URL}" | bash -s -- --tag "${MCP_ALPHA_VERIFIER_TAG}"`;
  const doctorCmd = 'x07-mcp-test doctor --machine json';
  const conformanceCmd =
    'x07-mcp-test conformance run --url "http://127.0.0.1:3000/mcp" --out out/conformance --machine json';

  return (
    <Layout
      title="Install (alpha)"
      description="Install the x07-mcp-test private alpha verifier binary.">
      <main className="container margin-vert--lg">
        <h1>Install x07-mcp-test (private alpha)</h1>
        <p>
          Prebuilt binaries for Linux and macOS. On Windows, run inside WSL2 and
          use the Linux artifact.
        </p>

        <h2>Install script</h2>
        <pre>
          <code>{installCmd}</code>
        </pre>
        <p>
          The installer downloads the correct archive for your OS/arch, verifies it via{' '}
          <a href={MCP_ALPHA_CHECKSUMS_URL}>checksums.txt</a>, and installs into{' '}
          <code>~/.local/bin</code>.
        </p>

        <h2>Manual download</h2>
        <ul>
          <li>
            Linux x64: <a href={mcpAlphaAssetUrl('linux-x64')}>{mcpAlphaAssetUrl('linux-x64')}</a>
          </li>
          <li>
            macOS arm64: <a href={mcpAlphaAssetUrl('darwin-arm64')}>{mcpAlphaAssetUrl('darwin-arm64')}</a>
          </li>
          <li>
            macOS x64: <a href={mcpAlphaAssetUrl('darwin-x64')}>{mcpAlphaAssetUrl('darwin-x64')}</a>
          </li>
          <li>
            Checksums: <a href={MCP_ALPHA_CHECKSUMS_URL}>{MCP_ALPHA_CHECKSUMS_URL}</a>
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
          <code>summary.json</code>, <code>summary.junit.xml</code>, <code>summary.html</code>.
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

