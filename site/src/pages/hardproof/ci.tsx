import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_ACTION_USES, HARDPROOF_TAG} from './_config';

export default function HardproofCi(): ReactNode {
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

      - name: Run Hardproof CI
        id: hardproof
        uses: ${HARDPROOF_ACTION_USES}
        with:
          url: http://127.0.0.1:3000/mcp
          allow-private-targets: "true"
          version: ${HARDPROOF_TAG}
          full-suite: "false"
          sarif: "true"
          threshold: "85"
          require-trust-for-full-score: "true"

      - name: Upload reports
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: hardproof-reports
          path: |
            out/doctor.json
            out/scan/**`;

  return (
    <Layout
      title="Hardproof in CI"
      description="Fail builds when MCP verification drops below your threshold.">
      <main className="container margin-vert--lg">
        <h1>Use Hardproof in CI</h1>
        <p>
          The GitHub Action downloads a <code>hardproof</code> release binary and runs{' '}
          <code>hardproof ci</code> (HTTP or stdio) against your server.
        </p>
        <p>
          <code>hardproof ci</code> runs <code>scan</code> and fails the job when verification fails
          or the score falls below <code>threshold</code>.
        </p>
        <p>
          For release-grade gating, require trust evidence as well so a partial score cannot pass as
          a publishable full score.
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
            <code>report_json</code>: <code>out/scan/scan.json</code>
          </li>
          <li>
            <code>report_sarif</code>: <code>out/scan/report.sarif.json</code> (when enabled)
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
