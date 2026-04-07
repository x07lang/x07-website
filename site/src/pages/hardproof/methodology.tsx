import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofMethodology(): ReactNode {
  return (
    <Layout
      title="Hardproof methodology"
      description="How Hardproof scores MCP servers: five deterministic dimensions plus usage metrics.">
      <main className="container margin-vert--lg">
        <h1>How Hardproof scores servers (methodology)</h1>

        <p>
          Hardproof is a deterministic verifier. It runs a fixed set of checks and emits a
          machine-readable scan report that humans can review and agents can consume directly.
        </p>

        <h2>Dimensions</h2>
        <p>The scan report includes five dimensions:</p>
        <ul>
          <li>
            <b>Conformance</b>: protocol behavior and required method coverage, using the official
            conformance baseline and optional fuller suites.
          </li>
          <li>
            <b>Security</b>: deterministic checks for transport exposure, descriptor drift,
            injection patterns, command risk, and auth posture.
          </li>
          <li>
            <b>Performance</b>: smoke, steady, and bounded-concurrency probes that stay cheap enough
            for CI while still producing usable latency and throughput signals.
          </li>
          <li>
            <b>Trust</b>: publisher identity, signature and transparency evidence, and bundle
            consistency when release artifacts are provided.
          </li>
          <li>
            <b>Reliability</b>: malformed input handling, replay stability, repeated-call drift, and
            other failure behavior that should stay reproducible across runs.
          </li>
        </ul>

        <h2>Score truth and publishability</h2>
        <p>
          Hardproof does not treat every numeric signal as equally publishable. The report makes
          score truth explicit:
        </p>
        <ul>
          <li>
            <code>publishable</code>: <code>overall_score</code> is present and backed by enough
            weighted dimensions.
          </li>
          <li>
            <code>partial</code>: <code>partial_score</code> is present, but <code>overall_score</code>{' '}
            stays <code>null</code> because at least one gate is still missing or unknown.
          </li>
          <li>
            <code>insufficient</code>: there is not enough evidence to defend a numeric score yet.
          </li>
        </ul>
        <p>
          Trust is the most common reason a scan remains partial. If you want full-score gating in
          CI, pass trust artifacts and enable <code>--require-trust-for-full-score</code>.
        </p>

        <h2>Usage metrics are an overlay</h2>
        <p>
          Usage metrics measure context pressure from tool catalogs, schemas, and response payloads.
          They are first-class in the report and CI policy, but they remain an overlay rather than a
          substitute for the five core verification dimensions.
        </p>

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
