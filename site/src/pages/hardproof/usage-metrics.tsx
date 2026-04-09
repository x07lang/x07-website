import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofUsageMetrics(): ReactNode {
  return (
    <Layout
      title="Hardproof usage metrics"
      description="Token and context usage metrics in Hardproof scan reports.">
      <main className="container margin-vert--lg">
        <h1>Token and context usage metrics</h1>

        <p>
          Hardproof includes a usage overlay in every scan report under <code>usage_metrics</code>.
          The overlay measures how much context a server consumes for an agent, especially around{' '}
          <code>tools/list</code>, schema payloads, and typical response sizes.
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
          The report includes byte counts and token counts for:
        </p>
        <ul>
          <li>
            <b>Tool catalog</b>: size of <code>tools/list</code> and its token footprint.
          </li>
          <li>
            <b>Descriptions and tool count</b>: average and max description size, plus overall tool
            count.
          </li>
          <li>
            <b>Schema footprint</b>: total tool input schema size and token footprint.
          </li>
          <li>
            <b>Response footprint</b>: typical response payload token footprint (p50/p95).
          </li>
          <li>
            <b>Metadata-to-payload ratio</b>: how much schema and descriptor overhead the server
            adds compared with the actual payload it returns.
          </li>
        </ul>

        <h2>Truth classes</h2>
        <p>
          There is no universal single “real token count” for an MCP server unless you either choose a
          tokenizer family or ingest a real client trace. Hardproof makes this explicit in{' '}
          <code>usage_metrics.usage_mode</code>:
        </p>
        <ul>
          <li>
            <code>usage_mode=estimate</code>: deterministic estimate fallback (default).
          </li>
          <li>
            <code>usage_mode=tokenizer_exact</code>: exact counts under a chosen tokenizer profile
            (for example <code>--tokenizer openai:o200k_base</code>).
          </li>
          <li>
            <code>usage_mode=trace_observed</code>: observed counts from a real client trace
            (<code>--token-trace</code>).
          </li>
          <li>
            <code>usage_mode=mixed</code>: per-metric mix of exact + observed when both are available.
          </li>
        </ul>

        <h2>Estimator metadata</h2>
        <p>
          In estimate mode, the usage overlay records <code>estimator_family</code>,{' '}
          <code>estimator_version</code>, and confidence next to the estimate fields. These values are
          deterministic comparison signals, not billing-grade truth.
        </p>

        <h2>Why two token estimates exist</h2>
        <p>
          The report keeps both <code>cl100k</code> and <code>o200k</code> tool-catalog estimates
          so consumers can compare context pressure across the model families that are commonly in
          use.
        </p>

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

        <h2>CI policy</h2>
        <p>
          Hardproof can gate on usage directly with thresholds such as{' '}
          <code>--max-avg-tool-description-tokens</code>, <code>--max-tool-count</code>, and{' '}
          <code>--max-metadata-to-payload-ratio-pct</code>.
        </p>

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
