import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofUsageMetrics(): JSX.Element {
  return (
    <Layout
      title="Hardproof usage metrics"
      description="Token and context usage metrics in Hardproof scan reports.">
      <main className="container margin-vert--lg">
        <h1>Token and context usage metrics</h1>

        <p>
          Hardproof includes a usage overlay in every scan report under <code>usage_metrics</code>.
          These metrics estimate how much context a server consumes for an agent, especially around{' '}
          <code>tools/list</code> and typical response payload sizes.
        </p>

        <h2>Why this exists</h2>
        <ul>
          <li>
            Oversized tool catalogs crowd out the actual user task from the model context.
          </li>
          <li>
            Oversized responses make agents brittle (truncation, high cost, hard-to-diff regressions).
          </li>
          <li>
            Schema bloat increases the steady-state prompt budget for every tool call.
          </li>
        </ul>

        <h2>What’s measured</h2>
        <p>
          The report includes estimated token counts for:
        </p>
        <ul>
          <li>
            <b>Tool catalog</b>: size of <code>tools/list</code> (bytes + estimated tokens).
          </li>
          <li>
            <b>Schema footprint</b>: total estimated tokens for tool input schemas.
          </li>
          <li>
            <b>Response footprint</b>: estimated response payload tokens (p50/p95).
          </li>
        </ul>

        <h2>How to keep usage healthy</h2>
        <ul>
          <li>
            Keep tool descriptions short and remove redundant examples.
          </li>
          <li>
            Prefer fewer tools with clearer names over many narrowly-scoped tools.
          </li>
          <li>
            Return only necessary fields; paginate and filter instead of returning “full objects”.
          </li>
        </ul>

        <h2>Next</h2>
        <ul>
          <li>
            Methodology: <Link to="/hardproof/methodology">/hardproof/methodology</Link>
          </li>
          <li>
            Report format: <Link to="/hardproof/report-format">/hardproof/report-format</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

