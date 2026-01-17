import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import {latestSchemasIndex} from './_agent_data';
import {AGENT_ENDPOINTS} from './_snippets';

export default function AgentSchemas(): ReactNode {
  return (
    <Layout title="Agent schemas" description="JSON schemas required by agents and tooling.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent schemas</Heading>
        <div className="alert alert--info">
          <strong>Machine-first index</strong>
          <ul>
            <li>
              <a href={AGENT_ENDPOINTS.schemas_index}>
                <code>{`GET ${AGENT_ENDPOINTS.schemas_index}`}</code>
              </a>
            </li>
          </ul>
        </div>

        <div className="table-responsive">
          <table className="table">
            <thead>
              <tr>
                <th>ID</th>
                <th>URL</th>
              </tr>
            </thead>
            <tbody>
              {latestSchemasIndex.items.map((it) => (
                <tr key={it.id}>
                  <td>
                    <code>{it.id}</code>
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
