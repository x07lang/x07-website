import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpSecurityGuide(): JSX.Element {
  return (
    <Layout
      title="MCP security and verification"
      description="Structure for the MCP Security Best Practices guide: how to combine official MCP guidance with repeatable verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>MCP security and verification (structure)</h1>

          <p>
            This page freezes the structure for the public MCP security/verification guide. The
            content will be drafted and iterated based on real verifier outputs.
          </p>

          <h2>Audience</h2>
          <ul>
            <li>MCP server maintainers who want repeatable release checks</li>
            <li>Platform/security teams that need machine-reviewable evidence</li>
          </ul>

          <h2>Threat model outline</h2>
          <ul>
            <li>Protocol compliance and undefined behavior</li>
            <li>Supply-chain risk (bundles, metadata drift, anti-rollback)</li>
            <li>Replayable evidence vs flaky integration tests</li>
          </ul>

          <h2>Practices (outline)</h2>
          <ul>
            <li>Run official conformance in CI (baseline)</li>
            <li>Record/replay a minimal cassette for release gates</li>
            <li>Verify trust metadata before publishing/consuming a server</li>
            <li>Validate bundle consistency (<code>server.json</code> ↔ <code>.mcpb</code>)</li>
          </ul>

          <h2>How this complements official MCP tools</h2>
          <ul>
            <li>Use Inspector for interactive debugging and protocol exploration.</li>
            <li>Use the official Registry for discovery/publishing metadata flows.</li>
            <li>
              Use <code>x07-mcp-test</code> for repeatable verification and artifacts.
            </li>
          </ul>

          <p>
            Related pages:{' '}
            <Link to="/mcp/quality-report">quality report</Link>,{' '}
            <Link to="/mcp/official-tools">official tools</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
