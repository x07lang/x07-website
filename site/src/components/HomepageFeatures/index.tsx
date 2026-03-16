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
    title: 'Agents can\'t hallucinate APIs that don\'t exist',
    description: (
      <>
        One canonical API per capability means there is exactly one right
        answer. No ambiguous choices, no wrong guesses, no import roulette.
      </>
    ),
  },
  {
    title: 'Errors tell agents exactly what to fix',
    description: (
      <>
        Every error carries a structured ID, a machine-readable explanation,
        and a suggested fix. Agents don't guess at repairs — they read and apply.
      </>
    ),
  },
  {
    title: 'From edit to production, one toolchain',
    description: (
      <>
        Formatter, linter, test runner, MCP bridge, WASM compiler, package
        registry, and deploy platform — all speaking the same typed contracts.
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
