import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function McpOfficialTools(): ReactNode {
  return (
    <Layout
      title="Official MCP tools"
      description="How official MCP tools, x07-mcp, and Hardproof fit together.">
      <main>
        <div className="container margin-vert--lg">
          <h1>Official MCP tools + Hardproof</h1>

          <p>
            The official MCP ecosystem gives you the protocol baseline and interactive tooling.
            X07 adds two focused layers on top: <code>x07-mcp</code> for authoring and bridge
            workflows, and <code>hardproof</code> for deterministic release verification.
          </p>

          <h2>What each tool is for</h2>
          <ul>
            <li>
              Official Inspector: interactive debugging and protocol exploration
            </li>
            <li>
              Official Registry flows: package discovery and publishing metadata
            </li>
            <li>
              <code>x07-mcp</code>: X07-native authoring, bridge, demos, and scaffolds
            </li>
            <li>
              <code>hardproof</code>: repeatable verification and CI artifacts
            </li>
          </ul>

          <h2>How the pieces fit together</h2>
          <ul>
            <li>
              Build and debug with the official SDKs and Inspector.
            </li>
            <li>
              Use <code>x07-mcp</code> when you want the server itself, or the bridge around the
              X07 toolchain, to be implemented in X07.
            </li>
            <li>
              Use <code>hardproof</code> in CI when you need deterministic scans, trust checks, and
              reviewable artifacts for a release decision.
            </li>
          </ul>

          <h2>Release-grade validation</h2>
          <ul>
            <li>
              Trust and bundle checks validate <code>server.json</code> and <code>.mcpb</code>
              artifacts.
            </li>
            <li>
              Scan reports keep a stable JSON contract for humans and agents to consume.
            </li>
          </ul>

          <p>
            Related pages:{' '}
            <Link to="/docs/toolchain/mcp-kit">MCP docs</Link>,{' '}
            <Link to="/hardproof/install">Hardproof install</Link>,{' '}
            <Link to="/hardproof/ci">Hardproof CI</Link>.
          </p>
        </div>
      </main>
    </Layout>
  );
}
