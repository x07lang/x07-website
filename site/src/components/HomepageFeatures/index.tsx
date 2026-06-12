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
    title: 'Deterministic execution',
    description: (
      <>
        Programs run in solve worlds with record/replay, so the same input
        produces the same output, the same diagnostics, and a rerunnable trace.
      </>
    ),
  },
  {
    title: 'Budgeted and capability-sandboxed',
    description: (
      <>
        Explicit budgets bound how long code runs and what it allocates;
        capability sandboxes decide what it can touch. Agent-written code runs
        inside limits you set, not limits it picks.
      </>
    ),
  },
  {
    title: 'Certifiable, not just plausible',
    description: (
      <>
        Spec-first XTAL tests, structured diagnostics with quickfixes, and
        proof-backed certification produce evidence a reviewer can check
        instead of take on faith.
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
