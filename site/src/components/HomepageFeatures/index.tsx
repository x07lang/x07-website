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
    title: 'Deterministic worlds',
    description: (
      <>
        Fixture worlds give repeatable tests and feedback loops. OS worlds are
        opt-in for production behavior.
      </>
    ),
  },
  {
    title: 'Canonical encodings',
    description: (
      <>
        Module boundaries use stable, pinned bytes contracts so agents don&apos;t
        guess formats or invent ad-hoc conventions.
      </>
    ),
  },
  {
    title: 'Repairable by design',
    description: (
      <>
        Diagnostics are stable and machine-readable, enabling automated patch
        application and reruns.
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
