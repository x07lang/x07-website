import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofMethodology(): JSX.Element {
  return (
    <Layout
      title="Hardproof methodology"
      description="How Hardproof scores MCP servers: five deterministic dimensions plus usage metrics.">
      <main className="container margin-vert--lg">
        <h1>How Hardproof scores servers (methodology)</h1>

        <p>
          Hardproof is a deterministic verifier. It runs a fixed set of checks and emits a
          machine-readable scan report that you can store, diff, and gate in CI.
        </p>

        <h2>Dimensions</h2>
        <p>The scan report includes five dimensions:</p>
        <ul>
          <li>
            <b>Conformance</b>: official MCP conformance outcomes (baseline vs full-suite).
          </li>
          <li>
            <b>Reliability</b>: repeated-request stability for core protocol calls.
          </li>
          <li>
            <b>Performance</b>: simple latency/concurrency probes that are CI-friendly.
          </li>
          <li>
            <b>Security</b>: deterministic checks for common tool-poisoning and request-confusion patterns.
          </li>
          <li>
            <b>Trust</b>: checks on registry metadata and bundle integrity when <code>server.json</code> /{' '}
            <code>.mcpb</code> inputs are provided.
          </li>
        </ul>

        <h2>Overall score and status</h2>
        <p>
          Hardproof reports an <code>overall_score</code> (0–100) when enough dimension scores are
          available. It also reports an overall <code>status</code>:
        </p>
        <ul>
          <li>
            <code>pass</code>: no critical findings and no warnings.
          </li>
          <li>
            <code>warn</code>: warnings exist (or some dimensions are unknown).
          </li>
          <li>
            <code>fail</code>: critical findings exist (or the CI policy fails).
          </li>
          <li>
            <code>unknown</code>: not enough data to compute a score (usually a precondition issue).
          </li>
        </ul>

        <h2>Evidence</h2>
        <p>
          Every scan includes findings (with codes, evidence, and suggested fixes) and references to
          generated artifacts. The report is designed to be reviewable without scraping logs.
        </p>

        <h2>Next</h2>
        <ul>
          <li>
            Report format: <Link to="/hardproof/report-format">/hardproof/report-format</Link>
          </li>
          <li>
            Usage metrics: <Link to="/hardproof/usage-metrics">/hardproof/usage-metrics</Link>
          </li>
          <li>
            Why deterministic: <Link to="/hardproof/deterministic">/hardproof/deterministic</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

