import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpSecurityGuide(): JSX.Element {
  const checklist = `# 1) Protocol compliance (baseline)
hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/scan --format json

# 2) Deterministic repro (release gate)
hardproof replay record --url "http://127.0.0.1:3000/mcp" --scenario smoke/basic --out out/replay.session.json --machine json
hardproof replay verify --session out/replay.session.json --url "http://127.0.0.1:3000/mcp" --out out/replay-verify --machine json

# 3) Supply-chain checks (publishing/consuming)
hardproof trust verify --server-json out/server.json --out out/trust.summary.json --machine json
hardproof bundle verify --server-json out/server.json --mcpb out/server.mcpb --out out/bundle.verify.json --machine json`;

  return (
    <Layout
      title="MCP security and verification"
      description="Structure for the MCP Security Best Practices guide: how to combine official MCP guidance with repeatable verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>MCP security and verification (draft)</h1>

          <p>
            Official MCP tools cover how to build and debug servers. This guide focuses on how to
            turn that work into repeatable verification evidence you can run on every release.
          </p>

          <h2>Audience</h2>
          <ul>
            <li>MCP server maintainers who want repeatable release checks</li>
            <li>Platform/security teams that need machine-reviewable evidence</li>
          </ul>

          <h2>Threat model (practical)</h2>
          <ul>
            <li>
              Protocol drift: servers that “mostly work” but fail in edge cases (bad errors,
              transport quirks, missing methods).
            </li>
            <li>
              Supply chain: mismatched metadata (<code>server.json</code>) versus what you actually
              ship (<code>.mcpb</code>).
            </li>
            <li>
              Non-reproducible bugs: failures that can’t be replayed deterministically in CI or
              code review.
            </li>
          </ul>

          <h2>Practices</h2>
          <ul>
            <li>
              <b>Conformance</b>: run the official suite in CI for every change.
            </li>
            <li>
              <b>Replay</b>: record a minimal cassette for release gates and regression review.
            </li>
            <li>
              <b>Trust</b>: verify registry metadata before publishing or consuming a server.
            </li>
            <li>
              <b>Bundle</b>: verify <code>server.json</code> ↔ <code>.mcpb</code> integrity.
            </li>
          </ul>

          <h2>How this complements official MCP tools</h2>
          <ul>
            <li>Use the official Inspector for interactive debugging and protocol exploration.</li>
            <li>Use the official Registry for discovery and publishing flows.</li>
            <li>Use <code>hardproof</code> for repeatable verification and CI artifacts.</li>
          </ul>

          <h2>Release checklist (example)</h2>
          <pre>
            <code>{checklist}</code>
          </pre>

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
