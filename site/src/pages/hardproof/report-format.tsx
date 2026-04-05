import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofReportFormat(): JSX.Element {
  const schemaId = 'https://x07.io/schemas/x07.mcp.scan.report.schema.json';
  const schemaVersion = 'x07.mcp.scan.report@0.3.0';
  const example = `{
  "schema_version": "${schemaVersion}",
  "tool": "hardproof",
  "tool_version": "v…",
  "report_kind": "scan",
  "target": { "kind": "mcp_server", "transport": "streamable_http", "ref": "…", "meta": {} },
  "status": "pass",
  "score_available": true,
  "overall_score": 92,
  "dimensions": [ /* conformance, reliability, performance, security, trust */ ],
  "usage_metrics": { /* token/context estimates */ },
  "findings": [ /* codes + evidence + suggested_fix */ ],
  "artifacts": [ /* referenced files (JSON/JUnit/HTML/SARIF, etc) */ ],
  "report_digest": "…",
  "run_id": "…"
}`;

  return (
    <Layout
      title="Hardproof report format"
      description="Report format for Hardproof scan reports (x07.mcp.scan.report@0.3.0).">
      <main className="container margin-vert--lg">
        <h1>Scan report format</h1>

        <p>
          Hardproof writes a scan report to <code>scan.json</code> (plus an event stream to{' '}
          <code>scan.events.jsonl</code>) under your chosen <code>--out</code> directory.
        </p>

        <h2>Schema</h2>
        <ul>
          <li>
            Schema version: <code>{schemaVersion}</code>
          </li>
          <li>
            JSON Schema: <a href={schemaId}>{schemaId}</a>
          </li>
        </ul>

        <h2>Top-level shape</h2>
        <pre>
          <code>{example}</code>
        </pre>

        <h2>Notes</h2>
        <ul>
          <li>
            <code>dimensions[]</code> contains the five dimension results; <code>usage_metrics</code> is
            an overlay focused on token/context footprint.
          </li>
          <li>
            <code>findings[]</code> is the stable place to look for actionable problems (codes, evidence, fixes).
          </li>
          <li>
            <code>artifacts[]</code> provides a review-friendly index of emitted files referenced by findings and dimensions.
          </li>
        </ul>

        <h2>Next</h2>
        <ul>
          <li>
            Methodology: <Link to="/hardproof/methodology">/hardproof/methodology</Link>
          </li>
          <li>
            Usage metrics: <Link to="/hardproof/usage-metrics">/hardproof/usage-metrics</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

