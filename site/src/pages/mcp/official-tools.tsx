import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpOfficialTools(): JSX.Element {
  return (
    <Layout
      title="Official MCP tools"
      description="Structure for the “official tools + Hardproof” positioning page: complement official Inspector/Registry with repeatable verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>Official MCP tools + Hardproof</h1>

          <p>
            The official MCP ecosystem gives you the protocol baseline and interactive tooling.
            <code>hardproof</code> complements that stack with CI-grade verification and
            reviewable artifacts.
          </p>

          <h2>Inspector vs verifier</h2>
          <ul>
            <li>Inspector: interactive debugging and protocol exploration</li>
            <li>
              <code>hardproof</code>: repeatable verification + CI artifacts (JSON/JUnit/HTML/SARIF)
            </li>
          </ul>

          <h2>Registry and publishing</h2>
          <ul>
            <li>Registry for discovery and publishing metadata flows</li>
            <li>
              <code>hardproof</code> trust/bundle checks for release-grade validation of{' '}
              <code>server.json</code> and <code>.mcpb</code>
            </li>
          </ul>

          <h2>How the pieces fit together</h2>
          <ul>
            <li>Build/debug with official SDKs + Inspector</li>
            <li>Verify in CI with <code>hardproof</code></li>
            <li>Publish/discover via Registry workflows</li>
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
