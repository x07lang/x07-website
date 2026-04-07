import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_ACTION_USES, HARDPROOF_FEEDBACK_URL} from './_config';

export default function HardproofFaq(): ReactNode {
  const scanCmd =
    'hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/scan';

  return (
    <Layout
      title="Hardproof FAQ"
      description="FAQ and positioning for the Hardproof public beta.">
      <main className="container margin-vert--lg">
        <h1>FAQ</h1>

        <h2>Do I need x07 to use Hardproof?</h2>
        <p>
          No. Hardproof is a standalone verifier for MCP servers. It is built with x07, but you do
          not need to adopt x07 to use it.
        </p>

        <h2>How is Hardproof related to x07-mcp?</h2>
        <p>
          Hardproof is the verifier path. <Link to="/mcp">x07-mcp</Link> is the x07-native authoring
          toolkit for building MCP servers in x07.
        </p>

        <h2>Is Hardproof replacing official MCP tools?</h2>
        <p>
          No. Official MCP tools provide the protocol baseline. Hardproof runs deterministic checks
          and emits machine-readable evidence you can use locally and in CI.
        </p>

        <h2>What’s the fastest way to use Hardproof?</h2>
        <pre>
          <code>{scanCmd}</code>
        </pre>

        <h2>Why is my score marked partial?</h2>
        <p>
          Partial means the scan is still useful, but not fully publishable yet. The common case is
          missing trust evidence, which keeps <code>overall_score</code> at <code>null</code> while
          <code>partial_score</code> still carries the numeric result.
        </p>

        <h2>GitHub Action</h2>
        <p>
          The Action is publicly branded as <b>Hardproof Scan (beta)</b>. Preferred usage is:
        </p>
        <pre>
          <code>uses: {HARDPROOF_ACTION_USES}</code>
        </pre>

        <h2>Where do I report issues?</h2>
        <p>
          Please file issues in <a href={HARDPROOF_FEEDBACK_URL}>{HARDPROOF_FEEDBACK_URL}</a>.
        </p>
      </main>
    </Layout>
  );
}
