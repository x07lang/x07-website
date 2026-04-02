import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpOfficialTools(): JSX.Element {
  return (
    <Layout
      title="Official MCP tools"
      description="Structure for the “official tools + x07-mcp-test” positioning page: complement official Inspector/Registry with repeatable verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>Official MCP tools + x07-mcp-test (structure)</h1>

          <p>
            This page freezes the structure and tone rules: x07 complements the official MCP
            ecosystem rather than replacing it.
          </p>

          <h2>Inspector vs verifier (outline)</h2>
          <ul>
            <li>Inspector: interactive debugging and protocol exploration</li>
            <li>
              <code>x07-mcp-test</code>: repeatable verification + CI artifacts (JSON/JUnit/HTML/SARIF)
            </li>
          </ul>

          <h2>Registry (outline)</h2>
          <ul>
            <li>Registry for discovery/publishing metadata flows</li>
            <li>
              <code>x07-mcp-test</code> trust/bundle checks for release-grade validation
            </li>
          </ul>

          <h2>How the pieces fit together</h2>
          <ul>
            <li>Debug with Inspector</li>
            <li>Verify in CI with <code>x07-mcp-test</code></li>
            <li>Publish via Registry workflows</li>
          </ul>

          <p>
            Related pages:{' '}
            <Link to="/mcp/install">install</Link>,{' '}
            <Link to="/mcp/action">GitHub Action</Link>,{' '}
            <Link to="/mcp/quality-report">quality report</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
