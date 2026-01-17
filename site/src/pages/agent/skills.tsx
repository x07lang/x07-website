import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import {latestSkillsIndex} from './_agent_data';
import {AGENT_ENDPOINTS} from './_snippets';

export default function AgentSkills(): ReactNode {
  return (
    <Layout title="Agent skills" description="Tooling skills available to coding agents.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent skills</Heading>
        <div className="alert alert--info">
          <strong>Machine-first index</strong>
          <ul>
            <li>
              <a href={AGENT_ENDPOINTS.skills_index}>
                <code>{`GET ${AGENT_ENDPOINTS.skills_index}`}</code>
              </a>
            </li>
          </ul>
        </div>

        <div className="table-responsive">
          <table className="table">
            <thead>
              <tr>
                <th>Skill</th>
                <th>Summary</th>
                <th>Output schema</th>
                <th>Docs</th>
              </tr>
            </thead>
            <tbody>
              {latestSkillsIndex.items.map((it) => (
                <tr key={it.id}>
                  <td>
                    <code>{it.id}</code>
                  </td>
                  <td>{it.summary}</td>
                  <td>
                    {it.report_schema_url ? (
                      <a href={it.report_schema_url}>{it.report_schema_url}</a>
                    ) : (
                      <span>—</span>
                    )}
                  </td>
                  <td>
                    <a href={it.docs_url}>{it.docs_url}</a>
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
