import type {ReactNode} from 'react';
import {useEffect, useMemo, useState} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

type SampleId = 'partial' | 'full';

type SampleConfig = {
  id: SampleId;
  title: string;
  description: string;
  scanUrl: string;
  eventsUrl: string;
  reportUrl: string;
  terminalUrl: string;
  liveRichVideoUrl: string;
  liveTuiVideoUrl: string;
};

const SAMPLES: SampleConfig[] = [
  {
    id: 'partial',
    title: 'Partial score (Trust not evaluated)',
    description:
      'Trust is missing, so overall_score stays null and the primary score is withheld in rich output.',
    scanUrl: '/hardproof/sample-reports/partial/scan.json',
    eventsUrl: '/hardproof/sample-reports/partial/scan.events.jsonl',
    reportUrl: '/hardproof/sample-reports/partial/report.html',
    terminalUrl: '/hardproof/sample-reports/partial/terminal.svg',
    liveRichVideoUrl: '/hardproof/sample-reports/partial/live-rich.webm',
    liveTuiVideoUrl: '/hardproof/sample-reports/partial/live-tui.webm',
  },
  {
    id: 'full',
    title: 'Full score (Trust evaluated)',
    description:
      'Trust inputs are present, so overall_score is publishable and rich output shows the full score.',
    scanUrl: '/hardproof/sample-reports/full/scan.json',
    eventsUrl: '/hardproof/sample-reports/full/scan.events.jsonl',
    reportUrl: '/hardproof/sample-reports/full/report.html',
    terminalUrl: '/hardproof/sample-reports/full/terminal.svg',
    liveRichVideoUrl: '/hardproof/sample-reports/full/live-rich.webm',
    liveTuiVideoUrl: '/hardproof/sample-reports/full/live-tui.webm',
  },
];

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null;
}

function asStringList(value: unknown): string[] {
  if (!Array.isArray(value)) return [];
  return value.filter((entry) => typeof entry === 'string') as string[];
}

function pretty(value: unknown): string {
  try {
    return JSON.stringify(value, null, 2);
  } catch {
    return String(value);
  }
}

export default function HardproofReportViewer(): ReactNode {
  const [sampleId, setSampleId] = useState<SampleId>('partial');
  const [scan, setScan] = useState<unknown | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);

  const sample = useMemo(() => {
    return SAMPLES.find((entry) => entry.id === sampleId) ?? SAMPLES[0];
  }, [sampleId]);

  useEffect(() => {
    let cancelled = false;
    setScan(null);
    setLoadError(null);

    (async () => {
      try {
        const res = await fetch(sample.scanUrl);
        if (!res.ok) {
          throw new Error(`HTTP ${res.status} while fetching ${sample.scanUrl}`);
        }
        const json = (await res.json()) as unknown;
        if (!cancelled) {
          setScan(json);
        }
      } catch (err) {
        if (!cancelled) {
          setLoadError(err instanceof Error ? err.message : String(err));
        }
      }
    })();

    return () => {
      cancelled = true;
    };
  }, [sample.scanUrl]);

  const scanDoc = isRecord(scan) ? scan : null;
  const dimensions = Array.isArray(scanDoc?.dimensions) ? (scanDoc?.dimensions as unknown[]) : [];
  const findings = Array.isArray(scanDoc?.findings) ? (scanDoc?.findings as unknown[]) : [];
  const usageMetrics = isRecord(scanDoc?.usage_metrics) ? scanDoc?.usage_metrics : null;
  const usageMode =
    typeof usageMetrics?.usage_mode === 'string' ? (usageMetrics.usage_mode as string) : 'estimate';
  const usageTokenizerId =
    typeof usageMetrics?.tokenizer_id === 'string' ? (usageMetrics.tokenizer_id as string) : '';
  const usageTraceSource =
    typeof usageMetrics?.trace_source === 'string' ? (usageMetrics.trace_source as string) : '';

  const overallScore = scanDoc?.overall_score;
  const scoreTruth = typeof scanDoc?.score_truth_status === 'string' ? scanDoc?.score_truth_status : '';
  const status = typeof scanDoc?.status === 'string' ? scanDoc?.status : '';
  const toolVersion = typeof scanDoc?.tool_version === 'string' ? scanDoc?.tool_version : '';
  const schemaVersion = typeof scanDoc?.schema_version === 'string' ? scanDoc?.schema_version : '';
  const scoreMode = typeof scanDoc?.score_mode === 'string' ? scanDoc?.score_mode : '';
  const partialScore = scanDoc?.partial_score;

  return (
    <Layout
      title="Hardproof report viewer"
      description="Static viewer for sample Hardproof scan reports (partial and full score).">
      <main className="container margin-vert--lg">
        <h1>Sample scan reports</h1>
        <p>
          This page renders two shipped sample scan reports: a partial score (Trust missing) and a
          publishable full score (Trust evaluated).
        </p>

        <h2>Choose a sample</h2>
        <p>
          <select
            value={sampleId}
            onChange={(event) => setSampleId(event.target.value as SampleId)}>
            {SAMPLES.map((entry) => (
              <option key={entry.id} value={entry.id}>
                {entry.title}
              </option>
            ))}
          </select>
        </p>
        <p>{sample.description}</p>

        <h2>Terminal snapshot</h2>
        <p>
          <a href={sample.terminalUrl}>{sample.terminalUrl}</a>
        </p>
        <img
          src={sample.terminalUrl}
          alt={`Hardproof ${sampleId} score terminal snapshot`}
          style={{maxWidth: '100%', borderRadius: 8, border: '1px solid var(--ifm-color-emphasis-300)'}}
        />

        <h2>Live scan clips</h2>
        <p className="text--muted">
          These short clips show the live scan UX for the same sample target (rich mode and alternate-screen TUI).
        </p>

        <h3>Live rich</h3>
        <p>
          <a href={sample.liveRichVideoUrl}>{sample.liveRichVideoUrl}</a>
        </p>
        <video
          src={sample.liveRichVideoUrl}
          controls
          playsInline
          preload="metadata"
          style={{maxWidth: '100%', borderRadius: 8, border: '1px solid var(--ifm-color-emphasis-300)'}}
        />

        <h3>Live TUI</h3>
        <p>
          <a href={sample.liveTuiVideoUrl}>{sample.liveTuiVideoUrl}</a>
        </p>
        <video
          src={sample.liveTuiVideoUrl}
          controls
          playsInline
          preload="metadata"
          style={{maxWidth: '100%', borderRadius: 8, border: '1px solid var(--ifm-color-emphasis-300)'}}
        />

        <h2>Report summary</h2>
        <ul>
          <li>
            schema_version: <code>{schemaVersion}</code>
          </li>
          <li>
            tool_version: <code>{toolVersion}</code>
          </li>
          <li>
            status: <code>{status}</code>
          </li>
          <li>
            score_truth_status: <code>{scoreTruth}</code>
          </li>
          <li>
            score_mode: <code>{scoreMode}</code>
          </li>
          <li>
            overall_score:{' '}
            <code>{overallScore === null || overallScore === undefined ? 'null' : String(overallScore)}</code>
          </li>
          <li>
            partial_score:{' '}
            <code>{partialScore === null || partialScore === undefined ? 'null' : String(partialScore)}</code>
          </li>
          <li>
            unknown_dimensions: <code>{pretty(asStringList(scanDoc?.unknown_dimensions))}</code>
          </li>
          <li>
            gating_reasons: <code>{pretty(asStringList(scanDoc?.gating_reasons))}</code>
          </li>
        </ul>

        <h2>Dimensions</h2>
        {dimensions.length === 0 ? (
          <p className="text--muted">No dimensions present in the sample report.</p>
        ) : (
          <div className="table-responsive">
            <table>
              <thead>
                <tr>
                  <th>dimension</th>
                  <th>status</th>
                  <th>score</th>
                  <th>weight</th>
                </tr>
              </thead>
              <tbody>
                {dimensions.map((dim, idx) => {
                  const dimDoc = isRecord(dim) ? dim : null;
                  const name = typeof dimDoc?.name === 'string' ? dimDoc?.name : `dim[${idx}]`;
                  const dimStatus = typeof dimDoc?.status === 'string' ? dimDoc?.status : '';
                  const dimScore = dimDoc?.score;
                  const dimWeight = dimDoc?.weight;
                  return (
                    <tr key={`${name}:${idx}`}>
                      <td>
                        <code>{name}</code>
                      </td>
                      <td>
                        <code>{dimStatus}</code>
                      </td>
                      <td>
                        <code>{dimScore === null || dimScore === undefined ? 'null' : String(dimScore)}</code>
                      </td>
                      <td>
                        <code>
                          {dimWeight === null || dimWeight === undefined ? 'null' : String(dimWeight)}
                        </code>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}

        <h2>Findings</h2>
        {findings.length === 0 ? (
          <p className="text--muted">No findings present in the sample report.</p>
        ) : (
          <ul>
            {findings.map((finding, idx) => {
              const f = isRecord(finding) ? finding : null;
              const code = typeof f?.code === 'string' ? f?.code : `finding[${idx}]`;
              const severity = typeof f?.severity === 'string' ? f?.severity : '';
              const title = typeof f?.title === 'string' ? f?.title : '';
              const summary = typeof f?.summary === 'string' ? f?.summary : '';
              const docsRef = typeof f?.docs_ref === 'string' ? f?.docs_ref : '';
              return (
                <li key={`${code}:${idx}`}>
                  <code>{code}</code> ({severity}) {title}
                  {summary ? ` — ${summary}` : ''}
                  {docsRef ? (
                    <>
                      {' '}
                      (<a href={docsRef}>{docsRef}</a>)
                    </>
                  ) : null}
                </li>
              );
            })}
          </ul>
        )}

        <h2>Usage overlay</h2>
        {usageMetrics ? (
          <>
            {usageMode === 'tokenizer_exact' ? (
              <p>
                Token values are exact under tokenizer profile{' '}
                <code>{usageTokenizerId || '(unknown)'}</code>.
              </p>
            ) : usageMode === 'trace_observed' ? (
              <p>
                Token values are observed from a client trace{' '}
                <code>{usageTraceSource || '(unknown)'}</code>.
              </p>
            ) : usageMode === 'mixed' ? (
              <p>
                Token values include a per-metric mix of exact + observed truth sources (tokenizer{' '}
                <code>{usageTokenizerId || '(unknown)'}</code>, trace{' '}
                <code>{usageTraceSource || '(unknown)'}</code>).
              </p>
            ) : (
              <p>
                Token values are deterministic estimates for comparison and gating, not billing-grade truth.
              </p>
            )}
            <pre>
              <code>{pretty(usageMetrics)}</code>
            </pre>
          </>
        ) : (
          <p className="text--muted">No usage_metrics present in the sample report.</p>
        )}

        <h2>Raw files</h2>
        <ul>
          <li>
            scan.json: <a href={sample.scanUrl}>{sample.scanUrl}</a>
          </li>
          <li>
            scan.events.jsonl: <a href={sample.eventsUrl}>{sample.eventsUrl}</a>
          </li>
          <li>
            report.html (raw JSON rendering): <a href={sample.reportUrl}>{sample.reportUrl}</a>
          </li>
          <li>
            terminal.svg: <a href={sample.terminalUrl}>{sample.terminalUrl}</a>
          </li>
          <li>
            live-rich.webm: <a href={sample.liveRichVideoUrl}>{sample.liveRichVideoUrl}</a>
          </li>
          <li>
            live-tui.webm: <a href={sample.liveTuiVideoUrl}>{sample.liveTuiVideoUrl}</a>
          </li>
        </ul>

        {loadError ? (
          <div className="alert alert--danger" role="alert">
            Failed to load sample scan report: <code>{loadError}</code>
          </div>
        ) : null}

        <h2>Next</h2>
        <ul>
          <li>
            Report schema: <Link to="/hardproof/report-format">/hardproof/report-format</Link>
          </li>
          <li>
            Methodology: <Link to="/hardproof/methodology">/hardproof/methodology</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}
