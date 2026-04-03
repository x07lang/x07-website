import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {MCP_CODESPACES_URL} from '../mcp/_config';

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
          <p>
            <b>Built with x07.</b>
          </p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/mcp/install">
              Install
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/action">
              CI
            </Link>
            <a className="button button--outline button--lg" href={MCP_CODESPACES_URL}>
              x07 MCP Codespaces
            </a>
          </div>
        </div>
      </header>
      <main>
        <div className="container margin-vert--lg">
          <p>
            Start here:{' '}
            <Link to="/mcp/install">install</Link>,{' '}
            <Link to="/mcp/action">CI</Link>,{' '}
            <Link to="/mcp/demo">demo</Link>,{' '}
            <Link to="/mcp/quality-report">quality report</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
