import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_ACTION_USES, HARDPROOF_FEEDBACK_URL} from './_config';

export default function HardproofFaq(): JSX.Element {
  const legacyCmd = 'x07-mcp-test conformance run --url "http://127.0.0.1:3000/mcp"';
  const newCmd = 'hardproof scan --url "http://127.0.0.1:3000/mcp" --out out/conformance --machine json';
  const legacyAction = 'x07lang/hardproof/action@v0.1.0-alpha.6';

  return (
    <Layout
      title="Hardproof FAQ"
      description="Compatibility, migration, and positioning for the Hardproof public beta.">
      <main className="container margin-vert--lg">
        <h1>FAQ / migration</h1>

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

        <h2>Migration from x07-mcp-test (beta)</h2>
        <p>
          Hardproof is the new public name for the private-alpha verifier previously released as{' '}
          <code>x07-mcp-test</code>. Legacy commands remain available during the beta transition.
        </p>
        <p>Legacy command example:</p>
        <pre>
          <code>{legacyCmd}</code>
        </pre>
        <p>Preferred command:</p>
        <pre>
          <code>{newCmd}</code>
        </pre>

        <h2>Migration for the GitHub Action (beta)</h2>
        <p>
          The Action is publicly branded as <b>Hardproof Scan (beta)</b>. Preferred usage is:
        </p>
        <pre>
          <code>uses: {HARDPROOF_ACTION_USES}</code>
        </pre>
        <p>
          If you previously used <code>{legacyAction}</code>, it remains available during the
          transition.
        </p>

        <h2>Where do I report issues?</h2>
        <p>
          Please file issues in <a href={HARDPROOF_FEEDBACK_URL}>{HARDPROOF_FEEDBACK_URL}</a>.
        </p>
      </main>
    </Layout>
  );
}

