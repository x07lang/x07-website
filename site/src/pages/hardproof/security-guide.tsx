import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

export default function HardproofSecurityGuide(): ReactNode {
  return (
    <Layout
      title="Hardproof security guide (draft)"
      description="Draft guide: what Hardproof Security does and does not claim today (deterministic-only).">
      <main className="container margin-vert--lg">
        <h1>Hardproof security guide (draft)</h1>

        <p>
          This guide documents the current <b>deterministic-only</b> Security dimension. It is not a
          penetration test, not a red-team report, and not a substitute for manual security review.
        </p>

        <h2>Deterministic-only stance</h2>
        <p>
          Hardproof Security is designed to produce findings that you can rerun and defend. It uses
          fixed probes and pattern checks and emits machine-readable evidence in the scan report.
        </p>

        <h2>What Security checks today</h2>
        <ul>
          <li>
            <b>Transport and auth exposure</b>: whether a remote HTTP endpoint is reachable without
            TLS and whether it responds without an auth challenge.
          </li>
          <li>
            <b>Host/Origin guard behavior</b>: whether a remote endpoint accepts mismatched Host and
            Origin headers (DNS rebinding surface).
          </li>
          <li>
            <b>Descriptor drift</b>: whether repeated <code>tools/list</code> results are stable.
          </li>
          <li>
            <b>Injection patterns</b>: heuristic pattern matches in tool metadata that increase
            prompt-injection surface.
          </li>
          <li>
            <b>Command-risk surface</b>: heuristic pattern matches for command-execution-adjacent
            terms in tool metadata and schemas that deserve manual review.
          </li>
          <li>
            <b>Descriptor bloat</b>: oversized tool descriptors that increase review burden and
            expand the injection surface.
          </li>
        </ul>

        <h2>What Security does not claim</h2>
        <ul>
          <li>Deep exploitation, adversarial chaining, or vulnerability confirmation.</li>
          <li>Non-deterministic fuzzing, load testing, or “LLM judge” evaluations.</li>
          <li>A guarantee that a server is secure because a scan produced no findings.</li>
        </ul>

        <h2>How to use Security findings</h2>
        <ul>
          <li>
            Treat findings as <b>review prompts</b>: each code is a concrete place to inspect.
          </li>
          <li>
            Prefer fixes that reduce surface area: shorten descriptors, remove imperative
            instructions from tool metadata, and keep schema examples minimal.
          </li>
          <li>
            Use <code>hardproof explain &lt;CODE&gt;</code> and <code>docs_ref</code> links to
            understand what was detected and what to change.
          </li>
        </ul>

        <h2>Next</h2>
        <ul>
          <li>
            Methodology: <Link to="/hardproof/methodology">/hardproof/methodology</Link>
          </li>
          <li>
            Usage overlay: <Link to="/hardproof/usage-metrics">/hardproof/usage-metrics</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

