import Layout from '@theme/Layout';

import {MCP_CODESPACES_URL} from './_config';

export default function McpCodespaces(): JSX.Element {
  const installVerifier = './scripts/dev/install_x07_mcp_test.sh';
  const runExample = `cd examples/private-alpha-http-hello
x07 bundle --project x07.json --profile os --out out/mcp-router
x07 bundle --project x07.json --profile sandbox --sandbox-backend os --i-accept-weaker-isolation --program src/worker_main.x07.json --out out/mcp-worker
./out/mcp-router`;
  const conformance = `x07-mcp-test conformance run \\
  --url "http://127.0.0.1:8314/mcp" \\
  --out out/conformance \\
  --machine json`;

  return (
    <Layout
      title="Codespaces"
      description="Zero-install evaluation path: open a Codespace, run a minimal X07 MCP server, and verify it locally with x07-mcp-test.">
      <main className="container margin-vert--lg">
        <h1>Try MCP verification with zero install (Codespaces)</h1>
        <p>
          Codespaces is the fastest way to evaluate the verifier without installing the x07 toolchain
          locally.
        </p>

        <h2>1) Open a Codespace</h2>
        <p>
          <a href={MCP_CODESPACES_URL}>{MCP_CODESPACES_URL}</a>
        </p>

        <h2>2) Install the verifier</h2>
        <pre>
          <code>{installVerifier}</code>
        </pre>

        <h2>3) Build and run the example server</h2>
        <pre>
          <code>{runExample}</code>
        </pre>
        <p>The MCP endpoint is <code>http://127.0.0.1:8314/mcp</code>.</p>

        <h2>4) Run conformance</h2>
        <pre>
          <code>{conformance}</code>
        </pre>
        <p>
          Artifacts are written under <code>out/conformance/</code>:{' '}
          <code>summary.json</code>, <code>summary.junit.xml</code>, <code>summary.html</code>.
        </p>
      </main>
    </Layout>
  );
}
