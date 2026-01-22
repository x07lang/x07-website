import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import {latestPackagesIndex} from './_agent_data';
import {AGENT_ENDPOINTS} from './_snippets';

export default function AgentPackages(): ReactNode {
  return (
    <Layout title="Agent packages" description="Machine-first indexes for official external packages.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent packages</Heading>
        <div className="alert alert--info">
          <strong>Machine-first index</strong>
          <ul>
            <li>
              <a href={AGENT_ENDPOINTS.packages_index}>
                <code>{`GET ${AGENT_ENDPOINTS.packages_index}`}</code>
              </a>
            </li>
          </ul>
        </div>

        <div className="table-responsive">
          <table className="table">
            <thead>
              <tr>
                <th>Package</th>
                <th>Version</th>
                <th>Index</th>
              </tr>
            </thead>
            <tbody>
              {latestPackagesIndex.items.map((it) => (
                <tr key={`${it.name}@${it.version}`}>
                  <td>
                    <code>{it.name}</code>
                  </td>
                  <td>
                    <code>{it.version}</code>
                  </td>
                  <td>
                    <a href={it.url}>{it.url}</a>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </main>
    </Layout>
  );
}

