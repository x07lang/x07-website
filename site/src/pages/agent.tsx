import type {ReactNode} from 'react';
import {useEffect, useMemo, useState} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import Link from '@docusaurus/Link';

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

export default function Agent(): ReactNode {
  const [versions, setVersions] = useState<ToolchainVersionsFile | null>(null);
  const [versionsError, setVersionsError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function load(): Promise<void> {
      try {
        const res = await fetch('/versions/toolchain_versions.json');
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

  return (
    <Layout
      title="Agent portal"
      description="Machine-first indexes, schemas, and deterministic entry points for coding agents.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent portal</Heading>
        <p>
          The agent surface is machine-first: stable URLs, JSON schemas, and a single manifest per
          toolchain release.
        </p>

        <Heading as="h2">Latest contracts</Heading>
        <ul>
          <li>
            <a href="/agent/latest/manifest.json">/agent/latest/manifest.json</a>
          </li>
          <li>
            <a href="/agent/latest/index.json">/agent/latest/index.json</a>
          </li>
          <li>
            <a href="/agent/latest/schemas/x07ast.schema.json">
              /agent/latest/schemas/x07ast.schema.json
            </a>
          </li>
          <li>
            <a href="/agent/latest/skills/pack/.codex/skills/README.md">
              /agent/latest/skills/pack/.codex/skills/README.md
            </a>
          </li>
        </ul>

        <Heading as="h2">Versioned contracts</Heading>
        <p>
          Source of truth:{' '}
          <a href="/versions/toolchain_versions.json">/versions/toolchain_versions.json</a>
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

        <Heading as="h2">Consumption rules</Heading>
        <ul>
          <li>
            Prefer <code>/agent/latest/manifest.json</code> unless your project pins a toolchain
            version.
          </li>
          <li>
            Use <code>/versions/toolchain_versions.json</code> to map toolchain versions to docs and
            contracts.
          </li>
          <li>
            Use <code>/docs/</code> for the human docs landing page; use <code>/docs/&lt;V&gt;/</code>{' '}
            for versioned human docs.
          </li>
        </ul>
      </main>
    </Layout>
  );
}
