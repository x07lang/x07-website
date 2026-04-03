import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpQualityReport(): JSX.Element {
  const corpusCmd = `hardproof corpus run \\
  --manifest corpus/manifests/quality-report-001.json \\
  --out out/corpus/quality-report-001`;

  const indexShape = `{
  "schema_version": "x07.mcp.corpus.summary@0.1.0",
  "generated_at": "2026-01-01T00:00:00Z",
  "tool": "hardproof",
  "tool_version": "v0.1.0-…",
  "target": { "kind": "file", "transport": "file", "ref": "…", "meta": {} },
  "ok": true,
  "errors": [],
  "warnings": [],
  "details": {
    "report_id": "quality-report-001",
    "counts": { "total": 1, "ok": 1, "failed": 0, "stub": 0 },
    "results": [
      {
        "server_id": "good-http",
        "status": "complete",
        "ok": true,
        "result_path": "good-http/result.json",
        "out_dir": "good-http"
      }
    ]
  }
}`;

  return (
    <Layout
      title="State of MCP server quality"
      description="Structure for the reproducible quality report pipeline: Hardproof corpus inputs → machine artifacts → publishable report surfaces.">
      <main>
        <div className="container margin-vert--lg">
          <h1>State of MCP server quality #1 (draft)</h1>

          <p>
            This report is designed to be reproducible: a checked-in corpus manifest defines the
            targets and assumptions, and <code>hardproof</code> emits machine-readable evidence
            (JSON + JUnit + HTML + SARIF) per target plus an aggregate <code>index.json</code>.
          </p>

          <h2>What this report is</h2>
          <ul>
            <li>
              A reproducible run of the official MCP conformance suite across a pinned set of
              servers and versions.
            </li>
            <li>
              A human-readable summary that links to raw evidence artifacts for review.
            </li>
          </ul>

          <h2>What this report is not</h2>
          <ul>
            <li>A security audit or a claim that a server is safe to run.</li>
            <li>A replacement for interactive debugging with the official MCP Inspector.</li>
          </ul>

          <h2>Inputs</h2>
          <ul>
            <li>
              Corpus manifest JSON (targets, sources, assumptions, exclusions).
            </li>
            <li>
              Per-target verifier outputs produced by <code>hardproof</code>.
            </li>
          </ul>

          <h2>Outputs</h2>
          <ul>
            <li>
              Aggregate index: <code>index.json</code> (schema{' '}
              <code>x07.mcp.corpus.summary@0.1.0</code>)
            </li>
            <li>
              Per-target result: <code>&lt;server_id&gt;/result.json</code> (schema{' '}
              <code>x07.mcp.corpus.result@0.1.0</code>)
            </li>
            <li>
              Per-target conformance artifacts: <code>summary.json</code>,{' '}
              <code>summary.junit.xml</code>, <code>summary.html</code>,{' '}
              <code>summary.sarif.json</code>
            </li>
            <li>CI-friendly artifacts (JUnit, SARIF)</li>
          </ul>

          <h2>Methodology (draft)</h2>
          <ul>
            <li>
              Target selection: representative, widely used servers (official + community) with
              clear version identifiers.
            </li>
            <li>
              Version pinning: record an explicit version (and commit hash when applicable) to make
              reruns comparable.
            </li>
            <li>
              Transport coverage: run against both Streamable HTTP and stdio where supported.
            </li>
            <li>
              Exclusions: any skipped scenarios are recorded explicitly in the manifest and results.
            </li>
          </ul>

          <h2>Reproducibility</h2>
          <p>
            Run the corpus runner locally:
          </p>
          <pre>
            <code>{corpusCmd}</code>
          </pre>

          <p>Expected aggregate shape (example):</p>
          <pre>
            <code>{indexShape}</code>
          </pre>

          <p>
            Related pages:{' '}
            <Link to="/mcp/security">security guide</Link>,{' '}
            <Link to="/mcp/official-tools">official tools</Link>,{' '}
            <Link to="/mcp/demo">Postgres demo</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
