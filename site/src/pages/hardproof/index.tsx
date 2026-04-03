import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_FEEDBACK_URL} from './_config';

export default function HardproofLanding(): JSX.Element {
  return (
    <Layout
      title="Hardproof"
      description="Ship MCP servers you can verify: deterministic verification for conformance, replay, trust, and release-grade evidence.">
      <header className="hero hero--primary">
        <div className="container">
          <h1>Ship MCP servers you can verify</h1>
          <p>
            Hardproof gives you deterministic verification for MCP servers: conformance, replay,
            trust, and release-grade evidence in one repeatable workflow.
          </p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/hardproof/install">
              Install Hardproof
            </Link>
            <Link className="button button--outline button--lg" to="/hardproof/ci">
              Use Hardproof in CI
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
              <b>Conformance</b>: run MCP conformance checks and emit JSON/JUnit/HTML/SARIF.
            </li>
            <li>
              <b>Replay</b>: record and replay a small HTTP session cassette.
            </li>
            <li>
              <b>Trust</b>: validate required trust metadata in registry <code>server.json</code>.
            </li>
            <li>
              <b>Bundle</b>: validate that a <code>server.json</code> and <code>.mcpb</code>{' '}
              bundle are consistent.
            </li>
          </ul>

          <h2>Relationship to x07</h2>
          <p>
            Hardproof is built with x07, but you do not need to adopt x07 to use it. If you want to
            build MCP servers in x07, start with <Link to="/mcp">x07-mcp</Link>.
          </p>

          <h2>Public beta status</h2>
          <p>
            This is a public-beta verifier with a tight scope: conformance plus replay/trust/bundle
            evidence. If you hit install friction or confusing output, please file an issue so we
            can fix it quickly.
          </p>
          <p>
            <a href={HARDPROOF_FEEDBACK_URL}>{HARDPROOF_FEEDBACK_URL}</a>
          </p>
        </div>
      </main>
    </Layout>
  );
}
