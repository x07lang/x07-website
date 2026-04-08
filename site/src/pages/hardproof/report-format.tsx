import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofReportFormat(): ReactNode {
  const schemaId = 'https://x07.io/schemas/x07.mcp.scan.report.schema.json';
  const schemaVersion = 'x07.mcp.scan.report@0.4.0';
  const toolVersion = '0.4.0-beta.2';
  const example = `{
  "schema_version": "${schemaVersion}",
  "tool": "hardproof",
  "tool_version": "${toolVersion}",
  "report_kind": "scan",
  "target": { "kind": "mcp_server", "transport": "streamable_http", "ref": "…", "meta": {} },
  "status": "warn",
  "score_available": true,
  "score_mode": "partial",
  "score_truth_status": "partial",
  "overall_score": null,
  "partial_score": 94,
  "dimension_coverage": {
    "conformance": true,
    "security": true,
    "performance": true,
    "reliability": true,
    "trust": false
  },
  "unknown_dimensions": ["trust"],
  "partial_reasons": ["TRUST-NOT-EVALUABLE", "SERVER-JSON-MISSING", "WEIGHT-COVERAGE-BELOW-FULL", "UNKNOWN-DIMENSIONS"],
  "gating_reasons": ["TRUST-NOT-EVALUABLE", "SERVER-JSON-MISSING", "WEIGHT-COVERAGE-BELOW-FULL", "UNKNOWN-DIMENSIONS"],
  "dimensions": [ /* conformance, reliability, performance, security, trust */ ],
  "usage_metrics": { /* token/context estimates + estimator_family/version/confidence */ },
  "findings": [ /* codes + evidence + suggested_fix */ ],
  "artifacts": [ /* referenced files (JSON/JUnit/HTML/SARIF, etc) */ ],
  "report_digest": "…",
  "run_id": "…"
}`;

  return (
    <Layout
      title="Hardproof report format"
      description="Report format for Hardproof scan reports (x07.mcp.scan.report@0.4.0).">
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
            <code>score_mode</code> is the dominant public truth field. Partial scans keep{' '}
            <code>overall_score</code> at <code>null</code>; <code>partial_score</code> remains a
            machine-readable comparison aid rather than the primary public score.
          </li>
          <li>
            Sample viewer: <Link to="/hardproof/report-viewer">/hardproof/report-viewer</Link>
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
