import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  MCP_ALPHA_CODESPACES_URL,
  MCP_ALPHA_FEEDBACK_URL,
} from './_config';

export default function McpLanding(): JSX.Element {
  return (
    <Layout
      title="MCP quality"
      description="Ship MCP servers you can verify: official conformance plus replay, trust, and release-grade validation.">
      <header className="hero hero--primary">
        <div className="container">
          <h1>Ship MCP servers you can verify</h1>
          <p>
            Official MCP gives you the protocol baseline. x07 adds replay,
            trust, and release-grade verification.
          </p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/mcp/install">
              Install the alpha verifier
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/codespaces">
              Try the x07 MCP quickstart in Codespaces
            </Link>
            <a className="button button--outline button--lg" href={MCP_ALPHA_FEEDBACK_URL}>
              Request private alpha access / send feedback
            </a>
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
              Zero-install evaluation: <Link to="/mcp/codespaces">/mcp/codespaces</Link>
            </li>
          </ul>

          <h2>Private alpha</h2>
          <p>
            This is a private alpha. If you hit install friction or confusing output, please
            file feedback so we can fix it before the public wedge launch.
          </p>
          <p>
            <a href={MCP_ALPHA_FEEDBACK_URL}>{MCP_ALPHA_FEEDBACK_URL}</a>
          </p>
          <p>
            Codespaces quickstart URL:{' '}
            <a href={MCP_ALPHA_CODESPACES_URL}>{MCP_ALPHA_CODESPACES_URL}</a>
          </p>
        </div>
      </main>
    </Layout>
  );
}

