import type {ReactNode} from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import HomepageFeatures from '@site/src/components/HomepageFeatures';
import styles from './index.module.css';

const ecosystemCards = [
  {
    eyebrow: 'MCP kit',
    title: 'Build MCP servers and use the official X07 MCP bridge',
    body:
      'Use x07-mcp to scaffold MCP servers in X07, or connect an agent runtime to the official x07lang-mcp server for structured editing, package, WASM, device, and lifecycle actions.',
    href: '/docs/toolchain/mcp-kit',
    cta: 'Open MCP docs',
  },
  {
    eyebrow: 'WASM',
    title: 'Ship browser UI, services, and device bundles from the same language',
    body:
      'The WASM toolchain covers pure modules, browser-hosted reducers, full-stack app bundles, and packaged desktop/mobile apps with one consistent build and test story.',
    href: '/docs/toolchain/wasm',
    cta: 'Open WASM docs',
  },
  {
    eyebrow: 'Platform',
    title: 'Run a lifecycle loop with packs, deploy plans, incidents, and regressions',
    body:
      'X07 Platform gives agents and operators one operational model for rollout control, incident capture, regression generation, and device release supervision.',
    href: '/docs/agent/platform',
    cta: 'Open platform docs',
  },
  {
    eyebrow: 'Registry',
    title: 'Publish packages and consume the official ecosystem through x07.io',
    body:
      'Use the registry to install pinned packages, publish your own, and keep machine-readable schemas and package metadata aligned with the public toolchain story.',
    href: 'https://x07.io',
    cta: 'Open x07.io',
  },
  {
    eyebrow: 'Docs',
    title: 'Learn from one source of truth for humans and coding agents',
    body:
      'x07lang.org serves the released docs bundle from the core repo and the agent portal exposes versioned schemas, skills, stdlib indexes, and examples.',
    href: '/docs/',
    cta: 'Browse docs',
  },
];

function HomepageHeader() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <div className={styles.heroKicker}>100% agent coding language</div>
        <img
          src="/img/logo-full-light.png"
          alt={siteConfig.title}
          className={clsx(styles.heroLogo, styles.heroLogoLight)}
        />
        <img
          src="/img/logo-full-dark.png"
          alt={siteConfig.title}
          className={clsx(styles.heroLogo, styles.heroLogoDark)}
        />
        <h1 className={styles.heroTitle}>
          Software that stays clear enough for humans and strict enough for
          coding agents
        </h1>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <p className={styles.heroLead}>
          X07 is an agent-first programming language with memory-safe defaults,
          structured concurrency, fast native execution, deterministic repair
          loops, and an official ecosystem for MCP, WASM, packages, and
          lifecycle operations.
        </p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs/getting-started/agent-quickstart">
            Start here
          </Link>
          <Link className="button button--outline button--lg" to="/docs/toolchain/mcp-kit">
            MCP kit
          </Link>
          <Link className="button button--outline button--lg" to="/docs/toolchain/wasm">
            WASM
          </Link>
          <Link className="button button--outline button--lg" to="/agent">
            Agent portal
          </Link>
        </div>
        <div className={styles.heroFacts}>
          <div className={styles.heroFact}>
            <strong>Memory-safe defaults</strong>
            <span>Checked values, explicit capabilities, no fragile patching.</span>
          </div>
          <div className={styles.heroFact}>
            <strong>Structured concurrency</strong>
            <span>One clear async model built for reliable agent edits.</span>
          </div>
          <div className={styles.heroFact}>
            <strong>Production surfaces</strong>
            <span>MCP, WASM, device, registry, and lifecycle tooling all line up.</span>
          </div>
        </div>
      </div>
    </header>
  );
}

export default function Home(): ReactNode {
  return (
    <Layout
      title="x07lang.org"
      description="X07 is an agent-first systems language with sandboxed execution and canonical contracts.">
      <HomepageHeader />
      <main>
        <section className={styles.ecosystemSection}>
          <div className="container">
            <div className={styles.sectionIntro}>
              <p className={styles.sectionEyebrow}>Ecosystem</p>
              <h2>One language, one story across the stack</h2>
              <p>
                Start with the core toolchain, then move into the official
                surfaces that matter for real software delivery.
              </p>
            </div>
            <div className={styles.cardGrid}>
              {ecosystemCards.map((card) => (
                <Link
                  key={card.title}
                  className={styles.card}
                  to={card.href}>
                  <p className={styles.cardEyebrow}>{card.eyebrow}</p>
                  <h3>{card.title}</h3>
                  <p>{card.body}</p>
                  <span className={styles.cardCta}>{card.cta}</span>
                </Link>
              ))}
            </div>
          </div>
        </section>
        <HomepageFeatures />
      </main>
    </Layout>
  );
}
