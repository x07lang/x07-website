import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  MCP_CODESPACES_URL,
} from './_config';

export default function McpLanding(): ReactNode {
  return (
    <Layout
      title="x07-mcp"
      description="The MCP authoring and bridge layer in the X07 ecosystem: build servers in X07, connect agents to X07 tooling, and pair them with Hardproof verification.">
      <header className="hero hero--primary">
        <div className="container">
          <h1>Build and connect MCP servers with X07</h1>
          <p>
            x07-mcp is the MCP lane in the X07 ecosystem: an authoring toolkit for X07-native
            servers, the official bridge for exposing X07 tooling to agent runtimes, and a clean
            handoff into Hardproof verification.
          </p>
          <div className="buttons">
            <Link className="button button--secondary button--lg" to="/docs/toolchain/mcp-kit">
              Open MCP docs
            </Link>
            <Link className="button button--outline button--lg" to="/hardproof">
              Verify with Hardproof
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/codespaces">
              Open Codespaces
            </Link>
            <Link className="button button--outline button--lg" to="/mcp/demo">
              Postgres demo
            </Link>
          </div>
        </div>
      </header>
      <main>
        <div className="container margin-vert--lg">
          <h2>How the pieces fit</h2>
          <ul>
            <li>
              <b>Build an MCP server in X07</b>: use x07-mcp to scaffold, run, bundle, and publish
              X07-native servers with a consistent architecture.
            </li>
            <li>
              <b>Connect agents to X07 tooling</b>: use the official x07lang-mcp bridge when you
              want typed MCP access to X07 toolchain operations from an agent runtime.
            </li>
            <li>
              <b>Verify any MCP server</b>: use <Link to="/hardproof">Hardproof</Link> to run
              deterministic verification and emit CI-grade artifacts with explicit score truth.
            </li>
          </ul>

          <h2>Start with one path</h2>
          <ul>
            <li>
              Zero-install evaluation: <Link to="/mcp/codespaces">/mcp/codespaces</Link>
            </li>
            <li>
              End-to-end demo: <Link to="/mcp/demo">/mcp/demo</Link>
            </li>
            <li>
              Build docs: <Link to="/docs/toolchain/mcp-kit">/docs/toolchain/mcp-kit</Link>
            </li>
            <li>
              Verification docs: <Link to="/hardproof/install">/hardproof/install</Link>
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
