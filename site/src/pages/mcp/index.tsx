import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  MCP_CODESPACES_URL,
  MCP_FEEDBACK_URL,
} from './_config';

export default function McpLanding(): JSX.Element {
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
            <Link className="button button--secondary button--lg" to="/mcp/install">
              Install Hardproof
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/demo">
              Run the Postgres demo
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/quality-report">
              See the quality report
            </Link>
          </div>
        </div>
      </header>
      <main>
        <div className="container margin-vert--lg">
          <h2>What it does</h2>
          <ul>
            <li>
              <b>Conformance</b>: run the official MCP suite and emit JSON/JUnit/HTML.
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

          <h2>Next steps</h2>
          <ul>
            <li>
              Install the verifier: <Link to="/mcp/install">/mcp/install</Link>
            </li>
            <li>
              Run in CI: <Link to="/mcp/action">/mcp/action</Link>
            </li>
            <li>
              Official tools positioning: <Link to="/mcp/official-tools">/mcp/official-tools</Link>
            </li>
            <li>
              Security + verification guide: <Link to="/mcp/security">/mcp/security</Link>
            </li>
            <li>
              Reproducible quality report: <Link to="/mcp/quality-report">/mcp/quality-report</Link>
            </li>
            <li>
              Zero-install evaluation: <Link to="/mcp/codespaces">/mcp/codespaces</Link>
            </li>
          </ul>

          <h2>Public beta status</h2>
          <p>
            This is a public-beta verifier with a tight scope: conformance plus replay/trust/bundle
            evidence. If you hit install friction or confusing output, please file an issue so we
            can fix it quickly.
          </p>
          <p>
            <a href={MCP_FEEDBACK_URL}>{MCP_FEEDBACK_URL}</a>
          </p>
          <p>
            Codespaces quickstart URL: <a href={MCP_CODESPACES_URL}>{MCP_CODESPACES_URL}</a>
          </p>
        </div>
      </main>
    </Layout>
  );
}
