import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpDemo(): ReactNode {
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

  const expectedFullScore = `+-----------------------------------------------+
| Hardproof - FULL SCORE                        |
+-----------------------------------------------+
| AI Infrastructure Score | Score Truth: [PASS] |
|                                               |
| target: http://127.0.0.1:8403/mcp             |
| transport: streamable_http                    |
| elapsed_ms: 14089ms                           |
| status: [WARN]                                |
| score: 93                                     |
| findings: critical 0, warning 2               |
+-----------------------------------------------+

== Dimensions ==
conformance  [########|#] 100 [PASS]
security     [########|#] 100 [PASS]
performance  [########~-] 83 [WARN]
trust        [########~-] 80 [WARN]
reliability  [########|#] 100 [PASS]

== Findings ==
warning:
- PERF-CONCURRENT-TOOLS-CALL-LOW: Low concurrent tool-call success
- TRUST-TRUSTPACK-MISSING: Trust pack metadata missing`;

  return (
    <Layout
      title="MCP demo"
      description="End-to-end demo: X07-native Postgres MCP server plus Hardproof verification artifacts.">
      <main>
        <div className="container margin-vert--lg">
          <h1>Postgres demo</h1>

          <p>
            This is the end-to-end demo path for the X07 MCP lane: run a real MCP server, then
            capture a deterministic Hardproof scan report plus replay, trust, and bundle evidence
            you can keep in CI or attach to a review.
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
              <b>Scan</b>: one report with five dimensions (conformance, reliability, performance,
              security, trust) plus token/context usage metrics
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

          <h2>Current hardened output (example)</h2>
          <p>
            This is an example rich summary from the demo’s publishable/full-score path (Trust
            evaluated). Exact timings and scores may vary slightly run-to-run.
          </p>
          <pre>
            <code>{expectedFullScore}</code>
          </pre>

          <p>
            The demo produces artifacts under <code>demos/postgres-public-beta/out/</code>, including{' '}
            <code>scan/scan.json</code>, <code>scan/scan.events.jsonl</code>,{' '}
            <code>replay.session.json</code>, <code>trust.summary.json</code>, and{' '}
            <code>bundle.verify.json</code>.
          </p>

          <p>
            Related pages:{' '}
            <Link to="/mcp/codespaces">Codespaces</Link>,{' '}
            <Link to="/hardproof/methodology">Hardproof methodology</Link>,{' '}
            <Link to="/hardproof/report-format">report format</Link>,{' '}
            <Link to="/hardproof/report-viewer">sample report viewer</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
