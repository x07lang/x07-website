import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofMethodology(): ReactNode {
  return (
    <Layout
      title="Hardproof methodology"
      description="How Hardproof scores MCP servers: five deterministic dimensions plus usage metrics.">
      <main className="container margin-vert--lg">
        <h1>How Hardproof scores servers (methodology)</h1>

        <p>
          Hardproof is a deterministic verifier. It runs a fixed set of checks and emits a
          machine-readable scan report that humans can review and agents can consume directly.
        </p>

        <h2>Dimensions</h2>
        <p>The scan report includes five dimensions:</p>
        <ul>
          <li>
            <b>Conformance</b>: protocol behavior and required method coverage, using the official
            conformance baseline and optional fuller suites.
          </li>
          <li>
            <b>Security</b>: deterministic checks for transport exposure, descriptor drift,
            injection patterns, command risk, and auth posture.
          </li>
          <li>
            <b>Performance</b>: smoke, steady, and bounded-concurrency probes that stay cheap enough
            for CI while still producing usable latency and throughput signals.
          </li>
          <li>
            <b>Trust</b>: publisher identity, signature and transparency evidence, and bundle
            consistency when release artifacts are provided.
          </li>
          <li>
            <b>Reliability</b>: malformed input handling, replay stability, repeated-call drift, and
            other failure behavior that should stay reproducible across runs.
          </li>
        </ul>

        <h2>Score truth (publishable vs partial)</h2>
        <p>
          Hardproof does not treat every numeric signal as equally publishable. The report makes
          score truth explicit.
        </p>
        <ul>
          <li>
            <code>score_truth_status=publishable</code> yields <code>score_mode=full</code>, where{' '}
            <code>overall_score</code> is present and backed by enough weighted dimensions.
          </li>
          <li>
            <code>score_truth_status=partial</code> yields <code>score_mode=partial</code>, where{' '}
            <code>overall_score</code> stays <code>null</code>, <code>partial_score</code> remains
            machine-readable as a comparison aid, and rich output withholds the primary score.
          </li>
          <li>
            <code>score_truth_status=insufficient</code> means there is not enough evidence to
            defend a numeric score yet.
          </li>
        </ul>
        <p>
          Trust is the most common reason a scan remains partial. <code>hardproof ci</code> now
          fails on partial scores by default. If you want full-score gating in CI, pass trust
          artifacts; use <code>--allow-partial-score</code> only when a partial result is
          intentional, and <code>--require-trust-for-full-score</code> when you want the strictest
          trust-aware gate.
        </p>

        <h2>Confidence and estimates</h2>
        <p>
          Hardproof tries to keep evidence and confidence separable. Some outputs are
          pass/fail-deterministic; others are bounded probes or deterministic estimates. Treat the
          report as a starting point for review, not as a substitute for judgment.
        </p>
        <ul>
          <li>
            <b>Usage metrics</b> (<code>usage_metrics</code>) are deterministic usage signals derived
            from byte counts by default, but the report can also include exact tokenization under a chosen
            tokenizer profile (<code>--tokenizer</code>) or observed usage from a real client trace
            (<code>--token-trace</code>). <code>usage_mode</code> makes the truth class explicit.
          </li>
          <li>
            <b>Performance probes</b> are bounded smoke/steady signals intended to stay cheap enough
            for CI. They include sample counts and a confidence marker (for example{' '}
            <code>tool_call_confidence</code>).
          </li>
          <li>
            <b>Score truth</b> is the public confidence boundary: partial scans keep{' '}
            <code>overall_score</code> at <code>null</code> until missing evidence gates (typically
            Trust) are satisfied.
          </li>
          <li>
            <b>Trust</b> requires release metadata inputs. Without trust artifacts, the Trust
            dimension stays unknown and the scan remains partial; with trust inputs, it can become
            publishable while still emitting warnings for missing transparency evidence.
          </li>
        </ul>

        <h2>Fairness and exclusions</h2>
        <p>
          Hardproof is intentionally scoped and deterministic. It does not attempt to answer every
          security or performance question, and its scores are not a cross-class leaderboard.
        </p>
        <ul>
          <li>
            <b>Exclusions</b>: deep exploitation, vulnerability confirmation, non-deterministic fuzzing,
            unconstrained load testing, and “LLM judge” evaluations.
          </li>
          <li>
            <b>Fair comparisons</b> require comparable conditions: same protocol baseline and suite,
            same transport, similar hardware/network, the same workload profile/budgets, and the
            same Trust inputs (or the same intentional absence of them).
          </li>
          <li>
            <b>Dimension boundaries</b>: Security findings include both hard checks (transport/auth
            exposure, Host/Origin guard behavior) and heuristic surface signals (injection patterns,
            command-risk patterns). Warnings are review prompts, not proofs of exploitation.
          </li>
        </ul>

        <h2>Usage metrics are an overlay</h2>
        <p>
          Usage metrics measure context pressure from tool catalogs, schemas, and response payloads.
          They are first-class in the report and CI policy, but they remain an overlay rather than a
          substitute for the five core verification dimensions.
        </p>

        <h2>Evidence</h2>
        <p>
          Every scan includes findings (with codes, evidence, and suggested fixes) and references to
          generated artifacts. The report is designed to be reviewable without scraping logs.
        </p>

        <h2>Next</h2>
        <ul>
          <li>
            Quality report: <Link to="/hardproof/quality-report">/hardproof/quality-report</Link>
          </li>
          <li>
            Report format: <Link to="/hardproof/report-format">/hardproof/report-format</Link>
          </li>
          <li>
            Security guide: <Link to="/hardproof/security-guide">/hardproof/security-guide</Link>
          </li>
          <li>
            Quality report pipeline (draft):{' '}
            <Link to="/hardproof/quality-report-pipeline">/hardproof/quality-report-pipeline</Link>
          </li>
          <li>
            Usage metrics: <Link to="/hardproof/usage-metrics">/hardproof/usage-metrics</Link>
          </li>
          <li>
            Why deterministic: <Link to="/hardproof/deterministic">/hardproof/deterministic</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}
