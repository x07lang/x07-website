import type {ReactNode} from 'react';
import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

type FeatureItem = {
  title: string;
  description: ReactNode;
};

const FeatureList: FeatureItem[] = [
  {
    title: 'Memory-safe defaults, not guesswork',
    description: (
      <>
        X07 keeps everyday code on checked abstractions, explicit capabilities,
        and canonical contracts. That removes a large class of “works until it
        doesn’t” systems-language surprises.
      </>
    ),
  },
  {
    title: 'Fast and concurrent without turning opaque',
    description: (
      <>
        Native code generation, structured concurrency, and budgeted execution
        give you performance and parallel work without sacrificing reviewability
        or reproducibility.
      </>
    ),
  },
  {
    title: 'Agent-grade tooling from edit to release',
    description: (
      <>
        The toolchain, MCP kit, WASM stack, package registry, and lifecycle
        platform all speak in structured reports and pinned contracts, so the
        same language story holds from local repair loops to production rollout.
      </>
    ),
  },
];

function Feature({title, description}: FeatureItem) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center padding-horiz--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures(): ReactNode {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
