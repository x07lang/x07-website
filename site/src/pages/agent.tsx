import type {ReactNode} from 'react';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import Link from '@docusaurus/Link';

export default function Agent(): ReactNode {
  return (
    <Layout
      title="Agent portal"
      description="Machine-first indexes, schemas, and deterministic entry points for coding agents.">
      <main className="container margin-vert--lg">
        <Heading as="h1">Agent portal</Heading>
        <p>
          The agent surface is machine-first: stable URLs, JSON schemas, and a single entry index.
        </p>
        <ul>
          <li>
            <Link href="/agent/latest/index.json" data-noBrokenLinkCheck>
              /agent/latest/index.json
            </Link>
          </li>
          <li>
            <Link href="/versions/toolchain_versions.json" data-noBrokenLinkCheck>
              /versions/toolchain_versions.json
            </Link>
          </li>
        </ul>
      </main>
    </Layout>
  );
}
