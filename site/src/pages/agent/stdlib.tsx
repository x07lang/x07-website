import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import {latestStdlibIndexMeta} from './_agent_data';
import {AGENT_ENDPOINTS} from './_snippets';

export default function AgentStdlib(): ReactNode {
  return (
    <Layout title="Agent stdlib index" description="Pinned stdlib module inventory for agents.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent stdlib index</Heading>
        <div className="alert alert--info">
          <strong>Machine-first index</strong>
          <ul>
            <li>
              <a href={AGENT_ENDPOINTS.stdlib_index}>
                <code>{`GET ${AGENT_ENDPOINTS.stdlib_index}`}</code>
              </a>
            </li>
          </ul>
        </div>

        <p>
          Latest stdlib index: <a href={latestStdlibIndexMeta.url}>{latestStdlibIndexMeta.url}</a>
        </p>
        <p>
          Modules: <code>{latestStdlibIndexMeta.module_count}</code>
        </p>
      </main>
    </Layout>
  );
}
