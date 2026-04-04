import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {
  HARDPROOF_FEEDBACK_URL,
  HARDPROOF_INSTALL_SH_URL,
  HARDPROOF_REPO,
  HARDPROOF_TAG,
} from './_config';

export default function HardproofLaunch(): JSX.Element {
  const installCmd = `curl -fsSL "${HARDPROOF_INSTALL_SH_URL}" | bash -s -- --tag "${HARDPROOF_TAG}"`;
  const scanCmd = 'hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/conformance --machine json';
  const ciCmd = 'hardproof ci --url "http://127.0.0.1:3000/mcp" --out out/conformance --machine json --threshold 80';
  const exampleSummaryUrl = `https://github.com/${HARDPROOF_REPO}/blob/${HARDPROOF_TAG}/docs/examples/hardproof-scan/summary.json`;

  return (
    <Layout
      title="Hardproof public beta"
      description="Introducing Hardproof: deterministic verification for MCP servers.">
      <main className="container margin-vert--lg">
        <h1>Hardproof public beta</h1>

        <p>
          <b>Hardproof is deterministic verification for MCP servers.</b>
        </p>
        <p>
          Conformance, replay, trust, and release-grade evidence in one repeatable workflow.{' '}
          <b>Built with x07.</b>
        </p>

        <div className="alert alert--info" role="alert">
          Beta tag: <code>{HARDPROOF_TAG}</code>
        </div>

        <h2>Install quickstart</h2>
        <pre>
          <code>{installCmd}</code>
        </pre>
        <pre>
          <code>{scanCmd}</code>
        </pre>

        <h2>CI quickstart</h2>
        <p>
          Use <code>hardproof ci</code> to fail builds when verification fails or drops below your
          threshold:
        </p>
        <pre>
          <code>{ciCmd}</code>
        </pre>
        <p>
          GitHub Actions: <Link to="/hardproof/ci">/hardproof/ci</Link>
        </p>

        <h2>Demo</h2>
        <p>
          Follow the Postgres demo (x07-native authoring + verification artifacts):{' '}
          <Link to="/mcp/demo">/mcp/demo</Link>
        </p>
        <p>
          Want to build MCP servers in x07? Start here: <Link to="/mcp">/mcp</Link>
        </p>

        <h2>Sample report</h2>
        <p>
          Canonical sample summary JSON:{' '}
          <a href={exampleSummaryUrl}>{exampleSummaryUrl}</a>
        </p>

        <h2>Feedback</h2>
        <p>
          Please file issues here: <a href={HARDPROOF_FEEDBACK_URL}>{HARDPROOF_FEEDBACK_URL}</a>
        </p>
      </main>
    </Layout>
  );
}

