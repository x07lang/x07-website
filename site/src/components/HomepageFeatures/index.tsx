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
    title: 'Humans and agents read the same contracts',
    description: (
      <>
        Docs, schemas, CLI reports, and package metadata line up, so the same
        contract survives from code review to CI to release operations.
      </>
    ),
  },
  {
    title: 'Canonical APIs cut down repair noise',
    description: (
      <>
        X07 tries to make each capability look like one obvious shape, which
        keeps generated code and repair patches more uniform and reviewable.
      </>
    ),
  },
  {
    title: 'Deterministic loops are easier to trust',
    description: (
      <>
        Lint, fix, test, run, MCP, packaging, and verification surfaces speak
        stable JSON and versioned schemas instead of ad hoc terminal output.
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
