import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import Link from '@docusaurus/Link';
import toolchainVersionsFile from '@site/static/versions/toolchain_versions.json';

import {
  latestExamplesIndex,
  latestPackagesIndex,
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

const TOOLCHAIN_VERSIONS = toolchainVersionsFile as ToolchainVersionsFile;

const GITHUB_RELEASES_URL = 'https://github.com/x07lang/x07/releases';

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

function githubReleaseTag(toolchainVersion: string): string {
  return `v${toolchainVersion}`;
}

function githubReleasePageUrl(toolchainVersion: string): string {
  return `${GITHUB_RELEASES_URL}/tag/${githubReleaseTag(toolchainVersion)}`;
}

function githubReleaseDownloadUrl(toolchainVersion: string, assetName: string): string {
  return `${GITHUB_RELEASES_URL}/download/${githubReleaseTag(toolchainVersion)}/${assetName}`;
}

function githubToolchainAssetName(
  toolchainVersion: string,
  platform: 'macOS' | 'Linux' | 'Windows',
): string {
  const base = `x07-v${toolchainVersion}-${platform}`;
  return platform === 'Windows' ? `${base}.zip` : `${base}.tar.gz`;
}

function githubSkillsPackAssetName(toolchainVersion: string): string {
  return `x07-skills-v${toolchainVersion}.tar.gz`;
}

function githubDocsBundleAssetName(toolchainVersion: string): string {
  return `x07-docs-v${toolchainVersion}.tar.gz`;
}

export default function AgentPortal(): ReactNode {
  const sorted = (TOOLCHAIN_VERSIONS.versions ?? []).slice().sort((a, b) => {
    const ak = semverKey(a.toolchain_version);
    const bk = semverKey(b.toolchain_version);
    if (!ak || !bk) return b.toolchain_version.localeCompare(a.toolchain_version);
    if (ak[0] !== bk[0]) return bk[0] - ak[0];
    if (ak[1] !== bk[1]) return bk[1] - ak[1];
    if (ak[2] !== bk[2]) return bk[2] - ak[2];
    return b.toolchain_version.localeCompare(a.toolchain_version);
  });

  const skillsCount = latestSkillsIndex.items.length;
  const schemasCount = latestSchemasIndex.items.length;
  const examplesCount = latestExamplesIndex.items.length;
  const packagesCount = latestPackagesIndex.items.length;

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
              <a href={AGENT_ENDPOINTS.packages_index}>
                <code>{`GET ${AGENT_ENDPOINTS.packages_index}`}</code>
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
            <Link to="/agent/packages">{`Packages (${packagesCount})`}</Link>
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
            and download the matching release assets (toolchain builds + skills pack + docs bundle)
            from{' '}
            <a href={GITHUB_RELEASES_URL}>
              <code>{GITHUB_RELEASES_URL}</code>
            </a>{' '}
            (see <Link to="/docs/getting-started/install">Install</Link>).
          </li>
          <li>
            Create a project with <code>x07 --init</code> (see{' '}
            <Link to="/docs/getting-started/first-project">Your first X07 program</Link>).
          </li>
          <li>
            Run lint: <code>x07 lint</code> (read <code>x07diag</code> JSON output).
          </li>
          <li>
            Apply JSON Patch fixes: <code>x07 fix</code> or <code>x07 ast apply-patch</code>.
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
          {TOOLCHAIN_VERSIONS.latest_toolchain_version ? (
            <>
              {' '}
              (latest toolchain: <code>{TOOLCHAIN_VERSIONS.latest_toolchain_version}</code>)
            </>
          ) : null}
        </p>
        {sorted.length ? (
          <div className="table-responsive">
            <table className="table">
              <thead>
                <tr>
                  <th>Toolchain</th>
                  <th>Human docs</th>
                  <th>Agent index</th>
                  <th>Agent manifest</th>
                  <th>Downloads</th>
                </tr>
              </thead>
              <tbody>
                {sorted.map((e) => {
                  const v = e.toolchain_version;
                  const releaseUrl = githubReleasePageUrl(v);
                  const macosUrl = githubReleaseDownloadUrl(v, githubToolchainAssetName(v, 'macOS'));
                  const linuxUrl = githubReleaseDownloadUrl(v, githubToolchainAssetName(v, 'Linux'));
                  const windowsUrl = githubReleaseDownloadUrl(v, githubToolchainAssetName(v, 'Windows'));
                  const skillsUrl = githubReleaseDownloadUrl(v, githubSkillsPackAssetName(v));
                  const docsUrl = githubReleaseDownloadUrl(v, githubDocsBundleAssetName(v));
                  const manifestUrl = githubReleaseDownloadUrl(v, 'release-manifest.json');
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
                      <td>
                        <a href={releaseUrl}>GitHub release</a>
                        <div>
                          <a href={macosUrl}>macOS</a> · <a href={linuxUrl}>Linux</a> ·{' '}
                          <a href={windowsUrl}>Windows</a> · <a href={skillsUrl}>Skills</a> ·{' '}
                          <a href={docsUrl}>Docs</a> · <a href={manifestUrl}>Manifest</a>
                        </div>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        ) : (
          <p>No toolchain versions are published yet.</p>
        )}
      </main>
    </Layout>
  );
}
