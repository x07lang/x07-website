import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  MCP_CODESPACES_URL,
} from './_config';

export default function McpLanding(): JSX.Element {
  return (
    <Layout
      title="x07-mcp"
      description="Build MCP servers in x07: templates, reference servers, and a Codespaces quickstart.">
      <header className="hero hero--primary">
        <div className="container">
          <h1>Build MCP servers in x07</h1>
          <p>
            x07-mcp is the x07-native Model Context Protocol toolkit: templates, reference servers,
            and a Codespaces path that gets you to a working server quickly.
          </p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/mcp/codespaces">
              Open Codespaces
            </Link>
            <Link className="button button--outline button--lg" to="/hardproof">
              Verify with Hardproof
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/demo">
              Postgres demo
            </Link>
          </div>
        </div>
      </header>
      <main>
        <div className="container margin-vert--lg">
          <h2>Two paths</h2>
          <ul>
            <li>
              <b>Verify any MCP server</b>: use <Link to="/hardproof">Hardproof</Link> to run
              deterministic verification and emit CI-grade artifacts.
            </li>
            <li>
              <b>Build an MCP server in x07</b>: use x07-mcp to scaffold, run, bundle, and publish
              x07-native servers with a consistent architecture.
            </li>
          </ul>

          <h2>Next steps</h2>
          <ul>
            <li>
              Start with the Codespaces quickstart: <Link to="/mcp/codespaces">/mcp/codespaces</Link>
            </li>
            <li>
              Follow the Postgres demo: <Link to="/mcp/demo">/mcp/demo</Link>
            </li>
            <li>
              Verify with Hardproof: <Link to="/hardproof/install">/hardproof/install</Link>
            </li>
          </ul>

          <p>
            Codespaces quickstart URL: <a href={MCP_CODESPACES_URL}>{MCP_CODESPACES_URL}</a>
          </p>
        </div>
      </main>
    </Layout>
  );
}
