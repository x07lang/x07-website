import type {ReactNode} from 'react';
import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';

import {HARDPROOF_REPO, HARDPROOF_TAG} from './_config';

const EVIDENCE_COMMIT = '04f5d7f4a63b606e1d8ac24cc39a20365a441e45';
const EVIDENCE_VERSION = '0.4.0-beta.2';

function hardproofBlobUrl(path: string): string {
  return `https://github.com/${HARDPROOF_REPO}/blob/${EVIDENCE_COMMIT}/${path}`;
}

export default function HardproofQualityReport(): ReactNode {
  const evidenceRoot = `release_qa/external_smoke_matrix/${EVIDENCE_VERSION}`;
  const evidenceReadme = hardproofBlobUrl('release_qa/external_smoke_matrix/README.md');
  const evidenceSummary = hardproofBlobUrl(`${evidenceRoot}/SUMMARY.md`);
  const evidenceMeta = hardproofBlobUrl(`${evidenceRoot}/meta.json`);
  const evidenceCommandLog = hardproofBlobUrl(`${evidenceRoot}/command.log`);
  const evidenceManifest = hardproofBlobUrl('release_qa/external_smoke_matrix/external-smoke-001.json');
  const evidenceScript = hardproofBlobUrl('scripts/release_qa/run_external_smoke_matrix.sh');

  const scanStdioUrl = hardproofBlobUrl(`${evidenceRoot}/stdio-x07lang-mcp/scan/scan.json`);
  const scanHttpPartialUrl = hardproofBlobUrl(`${evidenceRoot}/http-postgres-mcp-partial/scan/scan.json`);
  const scanHttpFullUrl = hardproofBlobUrl(`${evidenceRoot}/http-postgres-mcp-full/scan/scan.json`);

  const corpusReportUrl = hardproofBlobUrl(`${evidenceRoot}/corpus/report.html`);
  const corpusIndexUrl = hardproofBlobUrl(`${evidenceRoot}/corpus/index.json`);
  const corpusResultUrl = hardproofBlobUrl(`${evidenceRoot}/corpus/postgres-mcp-demo/result.json`);

  return (
    <Layout
      title="Hardproof quality report"
      description="First public Hardproof quality report: release QA evidence for real MCP targets (external smoke matrix).">
      <main className="container margin-vert--lg">
        <h1>Hardproof quality report (release QA)</h1>
        <p>
          This page is a public summary of the first recorded Hardproof release QA run against
          non-fixture targets. It is intended to be reproducible: every row links to the raw scan
          outputs and the exact manifest used.
        </p>

        <div className="alert alert--info" role="alert">
          Hardproof tag: <code>{HARDPROOF_TAG}</code> · Evidence: <code>{EVIDENCE_VERSION}</code> ·{' '}
          <a href={evidenceSummary}>SUMMARY.md</a>
        </div>

        <h2>Evidence bundle</h2>
        <ul>
          <li>
            How to rerun: <a href={evidenceReadme}>{evidenceReadme}</a> ·{' '}
            <a href={evidenceScript}>{evidenceScript}</a>
          </li>
          <li>
            Tool versions/commits: <a href={evidenceMeta}>{evidenceMeta}</a>
          </li>
          <li>
            Full command transcript: <a href={evidenceCommandLog}>{evidenceCommandLog}</a>
          </li>
          <li>
            Corpus manifest: <a href={evidenceManifest}>{evidenceManifest}</a>
          </li>
        </ul>

        <h2>Scan summary</h2>
        <p>
          Hardproof makes score truth explicit. Partial scans keep <code>overall_score</code> at{' '}
          <code>null</code>; full scans publish <code>overall_score</code> once the weighted evidence
          gates (typically Trust) are satisfied.
        </p>
        <div className="table-responsive">
          <table>
            <thead>
              <tr>
                <th>target</th>
                <th>status</th>
                <th>score_truth_status</th>
                <th>score_mode</th>
                <th>overall_score</th>
                <th>partial_score</th>
                <th>evidence</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td>
                  <code>stdio</code> x07lang-mcp
                </td>
                <td>
                  <code>fail</code>
                </td>
                <td>
                  <code>partial</code>
                </td>
                <td>
                  <code>partial</code>
                </td>
                <td>
                  <code>null</code>
                </td>
                <td>
                  <code>78</code>
                </td>
                <td>
                  <a href={scanStdioUrl}>scan.json</a>
                </td>
              </tr>
              <tr>
                <td>
                  <code>streamable_http</code> postgres-mcp demo (partial)
                </td>
                <td>
                  <code>warn</code>
                </td>
                <td>
                  <code>partial</code>
                </td>
                <td>
                  <code>partial</code>
                </td>
                <td>
                  <code>null</code>
                </td>
                <td>
                  <code>96</code>
                </td>
                <td>
                  <a href={scanHttpPartialUrl}>scan.json</a>
                </td>
              </tr>
              <tr>
                <td>
                  <code>streamable_http</code> postgres-mcp demo (full)
                </td>
                <td>
                  <code>warn</code>
                </td>
                <td>
                  <code>publishable</code>
                </td>
                <td>
                  <code>full</code>
                </td>
                <td>
                  <code>93</code>
                </td>
                <td>
                  <code>93</code>
                </td>
                <td>
                  <a href={scanHttpFullUrl}>scan.json</a>
                </td>
              </tr>
            </tbody>
          </table>
        </div>

        <h2>Corpus flow (public-sample pipeline)</h2>
        <p>
          The release QA run also records a minimal corpus run + render to prove the public-sample
          pipeline works end-to-end.
        </p>
        <ul>
          <li>
            Rendered corpus report: <a href={corpusReportUrl}>report.html</a>
          </li>
          <li>
            Corpus index: <a href={corpusIndexUrl}>index.json</a>
          </li>
          <li>
            Corpus result: <a href={corpusResultUrl}>result.json</a>
          </li>
        </ul>

        <h2>How to interpret this page</h2>
        <ul>
          <li>
            Methodology (scoring, confidence, exclusions):{' '}
            <Link to="/hardproof/methodology">/hardproof/methodology</Link>
          </li>
          <li>
            Report schema: <Link to="/hardproof/report-format">/hardproof/report-format</Link>
          </li>
          <li>
            Security guide: <Link to="/hardproof/security-guide">/hardproof/security-guide</Link>
          </li>
          <li>
            Pipeline outline (draft):{' '}
            <Link to="/hardproof/quality-report-pipeline">/hardproof/quality-report-pipeline</Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}

