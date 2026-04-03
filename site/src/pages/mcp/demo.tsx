import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpDemo(): JSX.Element {
  const cmds = `git clone https://github.com/x07lang/x07-mcp
cd x07-mcp/demos/postgres-public-beta

# 1) Start Postgres (Docker Compose)
./scripts/run_demo.sh --deps-only

# 2) Build + run the MCP server (in another terminal)
./scripts/run_demo.sh --server

# 3) Run verifier checks (in another terminal)
./scripts/verify_demo.sh

# 4) Copy outputs into a captured asset set
./scripts/capture_outputs.sh`;

  return (
    <Layout
      title="MCP demo"
      description="Structure for the public hero demo: Postgres MCP server + Hardproof verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>Postgres demo</h1>

          <p>
            This is the end-to-end “hero” demo path: run a real MCP server, then capture conformance
            + replay + trust + bundle evidence you can keep in CI or attach to a review.
          </p>

          <h2>What you run</h2>
          <ul>
            <li>
              The <code>postgres-mcp</code> reference server (x07-native MCP authoring path).
            </li>
            <li>A local Postgres via Docker Compose.</li>
          </ul>

          <h2>What you verify</h2>
          <ul>
            <li>
              <b>Conformance</b>: official MCP suite with JSON/JUnit/HTML/SARIF outputs
            </li>
            <li>
              <b>Replay</b>: record and replay a minimal deterministic cassette
            </li>
            <li>
              <b>Trust</b>: verify release metadata (<code>server.json</code>)
            </li>
            <li>
              <b>Bundle</b>: verify <code>server.json</code> ↔ <code>.mcpb</code> integrity
            </li>
          </ul>

          <h2>Command sequence</h2>
          <pre>
            <code>{cmds}</code>
          </pre>

          <p>
            The demo produces artifacts under <code>demos/postgres-public-beta/out/</code>, including
            <code>conformance/summary.json</code>, <code>replay.session.json</code>,{' '}
            <code>trust.summary.json</code>, and <code>bundle.verify.json</code>.
          </p>

          <p>
            Related pages:{' '}
            <Link to="/mcp/quality-report">quality report</Link>,{' '}
            <Link to="/mcp/security">security guide</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
