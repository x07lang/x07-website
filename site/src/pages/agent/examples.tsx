import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import {latestExamplesIndex} from './_agent_data';
import {AGENT_ENDPOINTS} from './_snippets';

export default function AgentExamples(): ReactNode {
  return (
    <Layout title="Agent examples" description="Example X07 programs for agents to copy.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent examples</Heading>
        <div className="alert alert--info">
          <strong>Machine-first index</strong>
          <ul>
            <li>
              <a href={AGENT_ENDPOINTS.examples_index}>
                <code>{`GET ${AGENT_ENDPOINTS.examples_index}`}</code>
              </a>
            </li>
          </ul>
        </div>

        <div className="table-responsive">
          <table className="table">
            <thead>
              <tr>
                <th>ID</th>
                <th>Purpose</th>
                <th>URL</th>
              </tr>
            </thead>
            <tbody>
              {latestExamplesIndex.items.map((it) => (
                <tr key={it.id}>
                  <td>
                    <code>{it.id}</code>
                  </td>
                  <td>{it.purpose}</td>
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
