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
    title: 'Built for AI agents',
    description: (
      <>
        Most languages optimize for human ergonomics. X07 optimizes for
        autonomous agents—one canonical API per capability, one encoding per
        format, zero ambiguity. Agents write correct code on the first try.
      </>
    ),
  },
  {
    title: 'Tests you can trust',
    description: (
      <>
        Fixture worlds provide deterministic testing: stable filesystems,
        virtual time, resource budgets. Same input, same output, every time.
        Agents get reliable feedback loops.
      </>
    ),
  },
  {
    title: 'Self-healing code',
    description: (
      <>
        Structured error IDs and machine-applicable fixes let agents
        automatically repair broken code. Diagnostics aren&apos;t just
        readable—they&apos;re actionable.
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
