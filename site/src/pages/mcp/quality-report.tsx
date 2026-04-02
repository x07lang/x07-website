import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpQualityReport(): JSX.Element {
  return (
    <Layout
      title="State of MCP server quality"
      description="Structure for the reproducible quality report pipeline: x07-mcp-test corpus inputs → machine artifacts → publishable report surfaces.">
      <main>
        <div className="container margin-vert--lg">
          <h1>State of MCP server quality #1 (structure)</h1>

          <p>
            This page freezes the public-beta information architecture for the first MCP quality
            report. It will be populated with real data and artifacts.
          </p>

          <h2>What this report is</h2>
          <ul>
            <li>
              A reproducible corpus run (targets + versions + exclusions) with machine-readable
              results.
            </li>
            <li>
              A human-readable summary that links to raw evidence (JSON, JUnit, SARIF, HTML).
            </li>
          </ul>

          <h2>What this report is not</h2>
          <ul>
            <li>A security audit or a claim that a server is safe to run.</li>
            <li>A replacement for official MCP Inspector-based debugging.</li>
          </ul>

          <h2>Inputs (frozen)</h2>
          <ul>
            <li>
              Corpus manifest JSON (targets, versions, exclusions, and run parameters).
            </li>
            <li>
              Per-target verifier outputs produced by <code>x07-mcp-test</code>.
            </li>
          </ul>

          <h2>Outputs (frozen)</h2>
          <ul>
            <li>Summary table + per-target drill-down</li>
            <li>Downloadable dataset bundle (raw JSON)</li>
            <li>CI-friendly artifacts (JUnit, SARIF)</li>
          </ul>

          <h2>Methodology outline (frozen)</h2>
          <ul>
            <li>Target selection policy and inclusion/exclusion criteria</li>
            <li>Version pinning and upgrade policy</li>
            <li>How failures are categorized (conformance vs runtime vs trust/bundle)</li>
            <li>Reproducibility contract (commands + inputs + expected output shape)</li>
          </ul>

          <h2>Reproducibility</h2>
          <p>
            The report pipeline is driven by <code>x07-mcp-test corpus run</code> and{' '}
            <code>x07-mcp-test corpus render</code>. The goal is to freeze the shapes that the
            website can rely on.
          </p>

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
