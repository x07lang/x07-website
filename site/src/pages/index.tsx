import type {ReactNode} from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import HomepageFeatures from '@site/src/components/HomepageFeatures';
import styles from './index.module.css';

const comparisonExamples = [
  {
    title: 'Reversing bytes should be boring',
    intro:
      'Python exposes several valid-looking ways to read stdin and manipulate bytes. X07 exposes one canonical bytes helper.',
    leftLabel: 'Python',
    leftMeta: 'Several plausible wrong turns',
    leftCode: `# Wrong turn 1 — text instead of bytes
import sys
data = sys.stdin.read()

# Wrong turn 2 — .reverse() does not exist on str
result = data.reverse()
# → AttributeError: 'str' has no attribute 'reverse'

# Wrong turn 3 — reversed(data) is an iterator
data = sys.stdin.buffer.read()
sys.stdout.buffer.write(reversed(data))
# → TypeError: a bytes-like object is required, not 'reversed'`,
    rightLabel: 'X07',
    rightMeta: 'One canonical bytes operation',
    rightCode: `{
  "schema_version": "x07.x07ast@0.4.0", // Canonical x07AST schema.
  "kind": "entry",
  "module_id": "main",
  "imports": ["std.bytes"],             // Import the bytes module.
  "decls": [],
  "solve": ["std.bytes.reverse", "input"] // Call the one reverse helper.
}`,
    footnote:
      'The X07 side is the shipped 03_reverse example. The point is not that Python cannot reverse bytes, but that X07 leaves the agent with far less API guesswork.',
  },
  {
    title: 'When the first attempt is wrong, the repair surface matters',
    intro:
      'Python gives the agent a human traceback. X07 gives it stable JSON diagnostics and deterministic repair commands.',
    leftLabel: 'Python',
    leftMeta: 'Human traceback',
    leftCode: `Traceback (most recent call last):
  File "reverse.py", line 6, in <module>
    sys.stdout.buffer.write(reversed(data))
TypeError: a bytes-like object is required, not 'reversed'`,
    rightLabel: 'X07',
    rightMeta: 'Machine-readable repair loop',
    rightCode: `x07 lint --input src/main.x07.json
# -> emits x07diag JSON with:
#    stable code
#    x07AST pointer
#    optional quickfix as JSON Patch

x07 fix --input src/main.x07.json --write
# -> applies the quickfix deterministically`,
    footnote:
      'The X07 docs define diagnostics as machine-readable, error codes as stable, and quickfixes as deterministic JSON Patch operations.',
  },
] as const;

const ecosystemCards = [
  {
    eyebrow: 'MCP kit',
    title: 'Give your agent structured access to every X07 tool',
    body:
      'Scaffold MCP servers in X07, or connect any agent runtime to the official x07lang-mcp bridge for editing, packaging, and deployment — all through typed, versioned contracts.',
    href: '/docs/toolchain/mcp-kit',
    cta: 'Open MCP docs',
  },
  {
    eyebrow: 'WASM',
    title: 'Ship to browser, server, and device from one codebase',
    body:
      'Compile to WebAssembly for browser UIs, backend services, or packaged desktop and mobile apps — with one build system and one test story.',
    href: '/docs/toolchain/wasm',
    cta: 'Open WASM docs',
  },
  {
    eyebrow: 'Platform',
    title: 'Automate the full lifecycle: deploy, monitor, repair',
    body:
      'One operational model for rollout control, incident tracking, regression testing, and device release supervision — built for agents and operators alike.',
    href: '/docs/agent/platform',
    cta: 'Open platform docs',
  },
  {
    eyebrow: 'Registry',
    title: 'Install and publish packages through x07.io',
    body:
      'Pinned dependencies, machine-readable metadata, and versioned schemas — all aligned with the toolchain so agents never guess at compatibility.',
    href: 'https://x07.io',
    cta: 'Open x07.io',
  },
  {
    eyebrow: 'Docs',
    title: 'One source of truth for humans and agents',
    body:
      'Every release ships docs for developers and a machine-first agent portal with versioned schemas, skills, stdlib indexes, and runnable examples.',
    href: '/docs/',
    cta: 'Browse docs',
  },
];

function HomepageHeader() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <div className={styles.heroKicker}>The agent-first language</div>
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
          AI agents guess when the language leaves room for guesswork.
          {' '}
          <span className={styles.heroHighlight}>
            x07 is designed so they don't have to.
          </span>
        </h1>
        <p className={styles.heroLead}>
          One canonical API per capability. Zero ambiguity. Agents get it right
          on the first try — with sandboxed execution, structured error IDs, and
          machine-applicable fixes.
        </p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs/getting-started/agent-quickstart">
            Start here
          </Link>
          <Link
            className="button button--outline button--lg"
            to="/docs/toolchain/mcp-kit">
            MCP kit
          </Link>
          <Link
            className="button button--outline button--lg"
            to="/docs/toolchain/wasm">
            WASM
          </Link>
          <Link
            className="button button--outline button--lg"
            to="/agent">
            Agent portal
          </Link>
        </div>
      </div>
    </header>
  );
}

function ComparisonSection() {
  return (
    <section className={styles.comparisonSection}>
      <div className="container">
        <div className={styles.sectionIntro}>
          <p className={styles.sectionEyebrow}>Why it matters</p>
          <h2>Less guesswork in the language. Less guesswork in the repair loop.</h2>
          <p>
            The first example is about API ambiguity. The second is about what
            happens when the first attempt is wrong.
          </p>
        </div>
        {comparisonExamples.map((example) => (
          <div key={example.title} className={styles.comparisonExample}>
            <div className={styles.comparisonExampleHeader}>
              <h3 className={styles.comparisonExampleTitle}>{example.title}</h3>
              <p className={styles.comparisonExampleIntro}>{example.intro}</p>
            </div>
            <div className={styles.comparisonGrid}>
              <div
                className={clsx(
                  styles.comparisonPanel,
                  styles.comparisonFail,
                )}>
                <div className={styles.comparisonLabel}>
                  <span className={styles.labelBadgeFail}>{example.leftLabel}</span>
                  <span className={styles.comparisonIter}>{example.leftMeta}</span>
                </div>
                <pre className={styles.comparisonCode}>
                  <code>{example.leftCode}</code>
                </pre>
              </div>
              <div
                className={clsx(
                  styles.comparisonPanel,
                  styles.comparisonPass,
                )}>
                <div className={styles.comparisonLabel}>
                  <span className={styles.labelBadgePass}>{example.rightLabel}</span>
                  <span className={styles.comparisonIter}>{example.rightMeta}</span>
                </div>
                <pre className={styles.comparisonCode}>
                  <code>{example.rightCode}</code>
                </pre>
              </div>
            </div>
            <p className={styles.comparisonFootnote}>{example.footnote}</p>
          </div>
        ))}
      </div>
    </section>
  );
}

export default function Home(): ReactNode {
  return (
    <Layout
      title="x07lang.org"
      description="AI agents guess when languages leave room for guesswork. X07 is designed with canonical APIs, structured errors, and machine-applicable fixes so agents do not have to.">
      <HomepageHeader />
      <main>
        <ComparisonSection />
        <section className={styles.ecosystemSection}>
          <div className="container">
            <div className={styles.sectionIntro}>
              <p className={styles.sectionEyebrow}>Ecosystem</p>
              <h2>One language, one story across the stack</h2>
              <p>
                Start with the core toolchain, then move into the official
                surfaces that ship real software.
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
