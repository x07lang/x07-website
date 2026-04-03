import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {MCP_ACTION_USES} from './_config';

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

      - name: Run Hardproof scan
        uses: ${MCP_ACTION_USES}
        with:
          url: http://127.0.0.1:3000/mcp
          full-suite: "false"

      - name: Upload reports
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: hardproof-reports
          path: |
            out/doctor.json
            out/conformance/summary.json
            out/conformance/summary.junit.xml
            out/conformance/summary.html
            out/conformance/summary.sarif.json
            out/conformance/summary.stdout.json`;

  return (
    <Layout
      title="GitHub Action"
      description="Run Hardproof scan in CI using the Hardproof GitHub Action.">
      <main className="container margin-vert--lg">
        <h1>Run Hardproof in CI</h1>
        <p>
          This Action downloads the <code>hardproof</code> release binary and runs <code>scan</code>{' '}
          against your server.
        </p>
        <p>
          During the beta transition, the Action is publicly branded as <b>Hardproof Scan</b> but is
          still served from the existing <code>x07lang/x07-mcp-test</code> repository path for
          compatibility.
        </p>

        <h2>Workflow snippet</h2>
        <pre>
          <code>{yaml}</code>
        </pre>

        <h2>Outputs</h2>
        <ul>
          <li>
            <code>ok</code>: true if scan passed (exit 0)
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
          <li>
            <code>sarif_report</code>: <code>out/conformance/summary.sarif.json</code>
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
