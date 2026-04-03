import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_ACTION_USES} from './_config';

export default function HardproofCi(): JSX.Element {
  const yaml = `name: hardproof

on:
  push:
  pull_request:

jobs:
  verify-mcp:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      # Start your MCP server here (or target a deployed URL).
      # - name: Start server
      #   run: ./scripts/start-server.sh

      - name: Run Hardproof scan
        id: hardproof
        uses: ${HARDPROOF_ACTION_USES}
        with:
          url: http://127.0.0.1:3000/mcp
          full-suite: "false"
          sarif: "true"

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
      title="Hardproof in CI"
      description="Fail builds when MCP verification drops below your threshold.">
      <main className="container margin-vert--lg">
        <h1>Use Hardproof in CI</h1>
        <p>
          The GitHub Action downloads a <code>hardproof</code> release binary and runs{' '}
          <code>hardproof scan</code> (HTTP or stdio) against your server.
        </p>
        <p>
          The public Action is called <b>Hardproof Scan (beta)</b>. Preferred usage is{' '}
          <code>x07lang/hardproof/hardproof-scan@…</code>; the legacy{' '}
          <code>x07lang/hardproof/action@…</code> path remains available during the transition.
        </p>

        <h2>Workflow snippet</h2>
        <pre>
          <code>{yaml}</code>
        </pre>

        <h2>Outputs</h2>
        <ul>
          <li>
            <code>scan_ok</code>: true if scan passed (exit 0)
          </li>
          <li>
            <code>report_json</code>: <code>out/conformance/summary.json</code>
          </li>
          <li>
            <code>report_junit</code>: <code>out/conformance/summary.junit.xml</code>
          </li>
          <li>
            <code>report_html</code>: <code>out/conformance/summary.html</code>
          </li>
          <li>
            <code>report_sarif</code>: <code>out/conformance/summary.sarif.json</code> (when enabled)
          </li>
        </ul>

        <h2>Next</h2>
        <ul>
          <li>
            Install locally: <Link to="/hardproof/install">/hardproof/install</Link>
          </li>
          <li>
            FAQ / migration: <Link to="/hardproof/faq">/hardproof/faq</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

