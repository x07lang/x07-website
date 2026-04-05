import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofDeterministic(): JSX.Element {
  return (
    <Layout
      title="Why deterministic verification"
      description="Why Hardproof is deterministic (not LLM-as-judge) for MCP server verification.">
      <main className="container margin-vert--lg">
        <h1>Why deterministic (not LLM-as-judge)</h1>

        <p>
          Hardproof is designed for verification that you can rerun and defend: the same inputs should
          produce the same report, with machine-readable evidence and explicit failure modes.
        </p>

        <h2>Deterministic verification gives you</h2>
        <ul>
          <li>
            <b>Reproducibility</b>: regressions are debuggable because the evidence is stable.
          </li>
          <li>
            <b>CI gating</b>: you can enforce policies (minimum score, zero critical findings) without
            flaky human interpretation.
          </li>
          <li>
            <b>Reviewability</b>: findings include codes, evidence, and suggested fixes; artifacts are indexed in the report.
          </li>
        </ul>

        <h2>What Hardproof is not</h2>
        <ul>
          <li>
            A substitute for manual security review or adversarial auditing.
          </li>
          <li>
            A “model judge” that changes its standards implicitly based on prompt drift.
          </li>
        </ul>

        <h2>Next</h2>
        <ul>
          <li>
            Methodology: <Link to="/hardproof/methodology">/hardproof/methodology</Link>
          </li>
          <li>
            Install: <Link to="/hardproof/install">/hardproof/install</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

