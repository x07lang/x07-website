import type {ReactNode} from 'react';
import {useEffect, useMemo, useState} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import Link from '@docusaurus/Link';

import {
  latestExamplesIndex,
  latestSchemasIndex,
  latestSkillsIndex,
  latestStdlibIndexMeta,
} from './_agent_data';
import {AGENT_ENDPOINTS, MINIMAL_AGENT_LOOP_COMMANDS} from './_snippets';

type ToolchainVersionEntry = {
  toolchain_version: string;
  docs_bundle_sha256?: string;
  published_at_utc?: string | null;
};

type ToolchainVersionsFile = {
  schema_version: string;
  latest_toolchain_version: string | null;
  versions: ToolchainVersionEntry[];
};

function semverKey(v: string): [number, number, number] | null {
  const parts = v.split('.');
  if (parts.length !== 3) return null;
  const major = Number(parts[0]);
  const minor = Number(parts[1]);
  const patch = Number(parts[2]);
  if (!Number.isInteger(major) || !Number.isInteger(minor) || !Number.isInteger(patch)) return null;
  if (major < 0 || minor < 0 || patch < 0) return null;
  return [major, minor, patch];
}

export default function AgentPortal(): ReactNode {
  const [versions, setVersions] = useState<ToolchainVersionsFile | null>(null);
  const [versionsError, setVersionsError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function load(): Promise<void> {
      try {
        const res = await fetch(AGENT_ENDPOINTS.toolchain_versions);
        if (!res.ok) {
          throw new Error(`HTTP ${res.status}`);
        }
        const data = (await res.json()) as ToolchainVersionsFile;
        if (cancelled) return;
        setVersions(data);
      } catch (e) {
        if (cancelled) return;
        setVersionsError(e instanceof Error ? e.message : String(e));
      }
    }

    void load();
    return () => {
      cancelled = true;
    };
  }, []);

  const sorted = useMemo(() => {
    const entries = versions?.versions ?? [];
    return entries.slice().sort((a, b) => {
      const ak = semverKey(a.toolchain_version);
      const bk = semverKey(b.toolchain_version);
      if (!ak || !bk) return b.toolchain_version.localeCompare(a.toolchain_version);
      if (ak[0] !== bk[0]) return bk[0] - ak[0];
      if (ak[1] !== bk[1]) return bk[1] - ak[1];
      if (ak[2] !== bk[2]) return bk[2] - ak[2];
      return b.toolchain_version.localeCompare(a.toolchain_version);
    });
  }, [versions]);

  const skillsCount = latestSkillsIndex.items.length;
  const schemasCount = latestSchemasIndex.items.length;
  const examplesCount = latestExamplesIndex.items.length;

  return (
    <Layout
      title="Agent portal"
      description="Machine-first indexes, schemas, and deterministic entry points for coding agents.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent portal</Heading>
        <p>
          Agents should consume JSON endpoints. The HTML pages under <code>/agent</code> are for
          discoverability and link to stable, machine-first contracts.
        </p>

        <div className="alert alert--info">
          <strong>Stable machine entrypoints</strong>
          <ul>
            <li>
              <a href={AGENT_ENDPOINTS.entrypoints}>
                <code>{`GET ${AGENT_ENDPOINTS.entrypoints}`}</code>
              </a>
            </li>
            <li>
              <a href={AGENT_ENDPOINTS.entry_index}>
                <code>{`GET ${AGENT_ENDPOINTS.entry_index}`}</code>
              </a>
            </li>
            <li>
              <a href={AGENT_ENDPOINTS.manifest}>
                <code>{`GET ${AGENT_ENDPOINTS.manifest}`}</code>
              </a>
            </li>
            <li>
              <a href={AGENT_ENDPOINTS.schemas_index}>
                <code>{`GET ${AGENT_ENDPOINTS.schemas_index}`}</code>
              </a>
            </li>
            <li>
              <a href={AGENT_ENDPOINTS.skills_index}>
                <code>{`GET ${AGENT_ENDPOINTS.skills_index}`}</code>
              </a>
            </li>
            <li>
              <a href={AGENT_ENDPOINTS.examples_index}>
                <code>{`GET ${AGENT_ENDPOINTS.examples_index}`}</code>
              </a>
            </li>
            <li>
              <a href={AGENT_ENDPOINTS.stdlib_index}>
                <code>{`GET ${AGENT_ENDPOINTS.stdlib_index}`}</code>
              </a>
            </li>
          </ul>
        </div>

        <Heading as="h2">Browse</Heading>
        <ul>
          <li>
            <Link to="/agent/skills">{`Skills (${skillsCount})`}</Link>
          </li>
          <li>
            <Link to="/agent/schemas">{`Schemas (${schemasCount})`}</Link>
          </li>
          <li>
            <Link to="/agent/examples">{`Examples (${examplesCount})`}</Link>
          </li>
          <li>
            <Link to="/agent/stdlib">{`Stdlib index (${latestStdlibIndexMeta.module_count} modules)`}</Link>
          </li>
        </ul>

        <Heading as="h2">Minimal agent loop</Heading>
        <ul>
          <li>
            Docs: <Link to="/docs/getting-started/agent-workflow">Agent workflow</Link>
          </li>
          <li>
            Docs: <Link to="/docs/toolchain/diagnostics-repair">Diagnostics &amp; repair</Link>
          </li>
          <li>
            URL contract: <Link to="/docs/site/agent-portal-contract">Agent portal contract</Link>
          </li>
        </ul>
        <pre>
          <code>{MINIMAL_AGENT_LOOP_COMMANDS.join('\n')}</code>
        </pre>

        <Heading as="h2">Start here</Heading>
        <ol>
          <li>
            Choose a toolchain version from{' '}
            <a href={AGENT_ENDPOINTS.toolchain_versions}>
              <code>{AGENT_ENDPOINTS.toolchain_versions}</code>
            </a>{' '}
            and install it (see <Link to="/docs/getting-started/install">Install</Link>).
          </li>
          <li>
            Create a program (see{' '}
            <Link to="/docs/getting-started/first-project">Your first X07 program</Link>).
          </li>
          <li>
            Run lint: <code>x07c lint</code> (read <code>x07diag</code> JSON output).
          </li>
          <li>
            Apply JSON Patch fixes: <code>x07c fix</code> or <code>x07c apply-patch</code>.
          </li>
          <li>
            Run tests: <code>x07 test</code> (read <code>x07test</code> JSON output).
          </li>
        </ol>

        <Heading as="h2" id="toolchains">
          Toolchains
        </Heading>
        <p>
          Source of truth:{' '}
          <a href={AGENT_ENDPOINTS.toolchain_versions}>
            <code>{AGENT_ENDPOINTS.toolchain_versions}</code>
          </a>
          {versions?.latest_toolchain_version ? (
            <>
              {' '}
              (latest toolchain: <code>{versions.latest_toolchain_version}</code>)
            </>
          ) : null}
        </p>
        {versionsError ? <p>Failed to load versions: {versionsError}</p> : null}
        {!versionsError && !versions ? <p>Loading versions…</p> : null}
        {versions ? (
          <div className="table-responsive">
            <table className="table">
              <thead>
                <tr>
                  <th>Toolchain</th>
                  <th>Human docs</th>
                  <th>Agent index</th>
                  <th>Agent manifest</th>
                </tr>
              </thead>
              <tbody>
                {sorted.map((e) => {
                  const v = e.toolchain_version;
                  return (
                    <tr key={v}>
                      <td>
                        <code>{v}</code>
                      </td>
                      <td>
                        <Link to={`/docs/${v}/`}>{`/docs/${v}/`}</Link>
                      </td>
                      <td>
                        <a href={`/agent/v${v}/index.json`}>{`/agent/v${v}/index.json`}</a>
                      </td>
                      <td>
                        <a href={`/agent/v${v}/manifest.json`}>{`/agent/v${v}/manifest.json`}</a>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        ) : null}
      </main>
    </Layout>
  );
}
