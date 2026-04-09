import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_FEEDBACK_URL} from './_config';

export default function HardproofLanding(): ReactNode {
  return (
    <Layout
      title="Hardproof"
      description="Deterministic verification for MCP servers: explicit score truth, machine-readable evidence, and review-friendly artifacts.">
      <header className="hero hero--primary">
        <div className="container">
          <h1>Verify MCP servers with evidence you can keep</h1>
          <p>
            Hardproof gives you deterministic verification for MCP servers: five dimensions, a
            usage overlay, explicit score truth, and stable artifacts you can store, diff, and gate
            in CI.
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
          <h2>What you get</h2>
          <ul>
            <li>
              <b>Scan report</b>: <code>hardproof scan</code> emits <code>scan.json</code> and{' '}
              <code>scan.events.jsonl</code> with five dimensions plus a usage overlay.
            </li>
            <li>
              <b>Live scan UX</b>: interactive terminals show live progress in rich mode and an
              alternate-screen TUI (<code>--ui rich</code> / <code>--ui tui</code>).
            </li>
            <li>
              <b>CI gate</b>: <code>hardproof ci</code> fails builds when verification fails or
              policy thresholds are crossed.
            </li>
            <li>
              <b>Replay and trust evidence</b>: replay sessions, trust summaries, and bundle checks
              keep releases reviewable and reproducible.
            </li>
            <li>
              <b>Rendered outputs</b>: export summary, HTML, and SARIF from an existing scan report
              with <code>hardproof report</code>.
            </li>
          </ul>

          <h2>Score truth</h2>
          <p>
            Hardproof separates a useful scan from a publishable full score.
          </p>
          <ul>
            <li>
              <b>Publishable</b>: <code>overall_score</code> is present and backed by enough
              weighted dimensions, including trust evidence when required.
            </li>
            <li>
              <b>Partial</b>: <code>overall_score</code> stays <code>null</code>,{' '}
              <code>partial_score</code> carries the numeric signal, and{' '}
              <code>partial_reasons</code> explains what is missing.
            </li>
            <li>
              <b>Insufficient</b>: the scan does not have enough evidence to defend any numeric
              score yet.
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
              Sample reports + live clips:{' '}
              <Link to="/hardproof/report-viewer">/hardproof/report-viewer</Link>
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
