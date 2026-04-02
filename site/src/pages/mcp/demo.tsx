import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpDemo(): JSX.Element {
  return (
    <Layout
      title="MCP demo"
      description="Structure for the public hero demo: Postgres MCP server + x07-mcp-test verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>Postgres demo (structure)</h1>

          <p>
            This page freezes the structure for the public demo: run one real MCP server end-to-end
            and show verifier evidence.
          </p>

          <h2>What you run</h2>
          <ul>
            <li>
              The <code>postgres-mcp</code> reference server (x07-native MCP authoring path).
            </li>
          </ul>

          <h2>What you verify</h2>
          <ul>
            <li>Conformance results</li>
            <li>Replay cassette + replay verification</li>
            <li>Trust metadata checks</li>
            <li>Bundle validation</li>
          </ul>

          <h2>Frozen command sequence</h2>
          <ul>
            <li>
              Start dependencies (Postgres), run the server, run{' '}
              <code>x07-mcp-test</code> commands, capture artifacts.
            </li>
          </ul>

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
