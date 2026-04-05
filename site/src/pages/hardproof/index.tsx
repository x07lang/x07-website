import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_FEEDBACK_URL} from './_config';

export default function HardproofLanding(): JSX.Element {
  return (
    <Layout
      title="Hardproof"
      description="Ship MCP servers you can verify: deterministic scoring and verification across five dimensions, plus token/context usage metrics.">
      <header className="hero hero--primary">
        <div className="container">
          <h1>Ship MCP servers you can verify</h1>
          <p>
            Hardproof gives you deterministic scoring and verification for MCP servers: five
            dimensions plus token/context usage metrics, emitted as a stable scan report you can
            store, diff, and gate in CI.
          </p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/hardproof/install">
              Install Hardproof
            </Link>
            <Link className="button button--outline button--lg" to="/hardproof/ci">
              Use Hardproof in CI
            </Link>
            <Link className="button button--outline button--lg" to="/hardproof/launch">
              Launch page
            </Link>
            <Link className="button button--outline button--lg" to="/hardproof/faq">
              FAQ / migration
            </Link>
          </div>
        </div>
      </header>
      <main>
        <div className="container margin-vert--lg">
          <h2>What it does</h2>
          <ul>
            <li>
              <b>Scan</b>: run <code>hardproof scan</code> and emit <code>scan.json</code> with
              five dimensions (conformance, reliability, performance, security, trust) plus a
              usage metrics overlay.
            </li>
            <li>
              <b>CI</b>: run <code>hardproof ci</code> to fail builds when verification fails or
              scores drop below your policy thresholds.
            </li>
            <li>
              <b>Replay</b>: record and replay a deterministic session cassette to keep regressions
              reproducible.
            </li>
            <li>
              <b>Reports</b>: render summaries and export SARIF/HTML from an existing scan report
              with <code>hardproof report</code>.
            </li>
          </ul>

          <h2>Learn more</h2>
          <ul>
            <li>
              How scoring works: <Link to="/hardproof/methodology">/hardproof/methodology</Link>
            </li>
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

          <h2>Relationship to x07</h2>
          <p>
            Hardproof is built with x07, but you do not need to adopt x07 to use it. If you want to
            build MCP servers in x07, start with <Link to="/mcp">x07-mcp</Link>.
          </p>

          <h2>Public beta status</h2>
          <p>
            This is a public-beta verifier. The report format is now stable enough to document, but
            the score methodology and thresholds may still evolve as we collect real-world feedback.
            If you hit install friction or confusing output, file an issue so we can fix it quickly.
          </p>
          <p>
            <a href={HARDPROOF_FEEDBACK_URL}>{HARDPROOF_FEEDBACK_URL}</a>
          </p>
        </div>
      </main>
    </Layout>
  );
}
