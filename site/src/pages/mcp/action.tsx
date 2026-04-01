import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {MCP_ALPHA_ACTION_USES} from './_config';

export default function McpAction(): JSX.Element {
  const yaml = `name: mcp-quality

on:
  push:
  pull_request:

jobs:
  conformance:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      # Start your MCP server here (or target a deployed URL).
      # - name: Start server
      #   run: ./scripts/start-server.sh

      - name: Run MCP conformance
        uses: ${MCP_ALPHA_ACTION_USES}
        with:
          url: http://127.0.0.1:3000/mcp
          full-suite: "false"

      - name: Upload reports
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: x07-mcp-test-reports
          path: |
            out/doctor.json
            out/conformance/summary.json
            out/conformance/summary.junit.xml
            out/conformance/summary.html
            out/conformance/summary.stdout.json`;

  return (
    <Layout
      title="GitHub Action (alpha)"
      description="Run official MCP conformance in CI using the x07-mcp-test alpha GitHub Action.">
      <main className="container margin-vert--lg">
        <h1>Run MCP conformance in CI (alpha)</h1>
        <p>
          This Action downloads the <code>x07-mcp-test</code> alpha release binary and runs{' '}
          <code>conformance run</code> against your server.
        </p>

        <h2>Workflow snippet</h2>
        <pre>
          <code>{yaml}</code>
        </pre>

        <h2>Outputs</h2>
        <ul>
          <li>
            <code>ok</code>: true if conformance passed (exit 0)
          </li>
          <li>
            <code>json_report</code>: <code>out/conformance/summary.json</code>
          </li>
          <li>
            <code>junit_report</code>: <code>out/conformance/summary.junit.xml</code>
          </li>
          <li>
            <code>html_report</code>: <code>out/conformance/summary.html</code>
          </li>
        </ul>

        <h2>Next</h2>
        <ul>
          <li>
            Install locally: <Link to="/mcp/install">/mcp/install</Link>
          </li>
          <li>
            Try the zero-install path: <Link to="/mcp/codespaces">/mcp/codespaces</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

