import type {ReactNode} from 'react';
import Layout from '@theme/Layout';

export default function HardproofQualityReportPipeline(): ReactNode {
  return (
    <Layout
      title="Hardproof quality report pipeline (draft)"
      description="Draft outline: data flow and methodology appendix sections for a future Hardproof quality report.">
      <main className="container margin-vert--lg">
        <h1>Hardproof quality report pipeline (draft)</h1>

        <p>
          This is a draft outline for the first quality report pipeline. It is scaffolding for
          methodology and reproducible aggregation, <b>not a public proof page yet</b>.
        </p>

        <h2>Data flow (proposed)</h2>
        <ol>
          <li>
            <b>Corpus selection</b>: a checked-in manifest describing which targets are in-scope,
            with explicit exclusions and rationale.
          </li>
          <li>
            <b>Run manifests</b>: pinned tool versions, baseline versions, and deterministic scan
            inputs (including trust inputs when required).
          </li>
          <li>
            <b>Execution</b>: run <code>hardproof corpus run</code> to produce a stable per-target
            directory of scan artifacts.
          </li>
          <li>
            <b>Aggregation</b>: an offline script/notebook that computes summary tables and charts
            from corpus outputs without scraping logs.
          </li>
          <li>
            <b>Publication</b>: a report viewer page that embeds the methodology appendix and
            links to reproducible artifacts.
          </li>
        </ol>

        <h2>Methodology appendix (sections)</h2>
        <ul>
          <li>
            <b>Score truth</b>: publishable vs partial vs insufficient, and what evidence is required
            before a number is treated as publishable.
          </li>
          <li>
            <b>Confidence</b>: which signals are estimate-grade (usage metrics) and which probes are
            bounded smoke signals (performance), including any confidence markers and sample counts.
          </li>
          <li>
            <b>Fairness</b>: what comparisons are and are not meaningful across different server
            classes and transports.
          </li>
          <li>
            <b>Determinism</b>: what makes corpus runs reproducible and what is intentionally
            excluded (non-deterministic load testing, LLM judging).
          </li>
          <li>
            <b>Interpretation</b>: how to interpret warnings vs failures vs partial scores, and what
            follow-up evidence is expected before making stronger claims.
          </li>
        </ul>
      </main>
    </Layout>
  );
}

