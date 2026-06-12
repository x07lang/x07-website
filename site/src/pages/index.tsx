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

const authoringFeatures = [
  {
    eyebrow: 'x07text',
    title: 'Lossless text projection',
    body: (
      <>
        <code>x07 ast to-text</code> and <code>x07 ast from-text</code>{' '}
        round-trip between canonical x07AST JSON and a readable, writable text
        form, so agents and humans can author X07 directly without losing
        structure.
      </>
    ),
  },
  {
    eyebrow: 'x07 doc',
    title: 'Behavioral API summaries',
    body: (
      <>
        <code>x07 doc</code> describes what stdlib and package functions do —
        inputs, outputs, and effects — so authors guess less before the first
        compile.
      </>
    ),
  },
  {
    eyebrow: 'Diagnostics',
    title: 'Did-you-mean suggestions',
    body: (
      <>
        Near-miss identifiers come back as structured diagnostics with concrete
        suggestions, which keeps the first repair iteration short.
      </>
    ),
  },
] as const;

const ecosystemCards = [
  {
    eyebrow: 'Core docs',
    title: 'Understand the language, worlds, toolchain, and ecosystem',
    body:
      'Start at the docs if you are evaluating X07, installing the toolchain, or trying to understand how the pieces fit together before you build anything.',
    href: '/docs/',
    cta: 'Read the docs',
  },
  {
    eyebrow: 'MCP',
    title: 'Author servers in X07 or connect agents through the official bridge',
    body:
      'Use x07-mcp to build MCP servers in X07, run demos, or connect agent runtimes to the official x07lang-mcp bridge for typed access to X07 tooling.',
    href: '/docs/toolchain/mcp-kit',
    cta: 'Open MCP docs',
  },
  {
    eyebrow: 'WASM',
    title: 'Compile the same programs to a wasm target',
    body:
      'x07-wasm-backend compiles X07 programs to WebAssembly for environments where a native binary is not an option, with the same determinism and test story.',
    href: '/docs/toolchain/wasm',
    cta: 'Open WASM docs',
  },
  {
    eyebrow: 'Hardproof',
    title: 'Verify MCP servers with evidence you can keep in CI and review',
    body:
      'Hardproof is the verification lane: deterministic scans, explicit score truth, replay artifacts, trust checks, and usage metrics for MCP servers.',
    href: '/hardproof',
    cta: 'Open Hardproof',
  },
  {
    eyebrow: 'Registry',
    title: 'Install and publish packages through x07.io',
    body:
      'Pinned dependencies, machine-readable metadata, and versioned schemas stay aligned with the toolchain so humans and agents do not guess at compatibility.',
    href: 'https://x07.io',
    cta: 'Open x07.io',
  },
];

const quickStartPaths = [
  {
    eyebrow: 'For humans',
    title: 'Start with the docs and ecosystem overview',
    body:
      'Use this path if you are opening X07 for the first time and want a clear explanation of the execution model, the toolchain, and where MCP, the wasm target, packages, and certification fit.',
    href: '/docs/',
    cta: 'Read docs',
  },
  {
    eyebrow: 'For builders',
    title: 'Build or verify MCP servers',
    body:
      'Use x07-mcp to author servers in X07 and Hardproof to verify any MCP server with deterministic reports, CI gates, and review artifacts.',
    href: '/mcp',
    cta: 'Open MCP lane',
  },
  {
    eyebrow: 'For agents',
    title: 'Consume stable machine entrypoints',
    body:
      'Use the agent portal for versioned schemas, skills, stdlib indexes, package indexes, examples, and release manifests. The HTML pages are discoverability only.',
    href: '/agent',
    cta: 'Open agent portal',
  },
] as const;

function HomepageHeader() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <div className={styles.heroKicker}>Code generation is getting cheap. Trust is the bottleneck.</div>
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
          X07 is the deterministic, certifiable execution substrate
          {' '}
          <span className={styles.heroHighlight}>
            for agent-written software.
          </span>
        </h1>
        <p className={styles.heroLead}>
          X07 runs agent-written code deterministically — solve worlds,
          record/replay — under explicit budgets and capability sandboxes, with
          structured diagnostics and quickfixes, spec-first testing (XTAL), and
          proof-backed certification. The C backend compiles fast to small
          native binaries, with a wasm target.
        </p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs/">
            Read docs
          </Link>
          <Link
            className="button button--outline button--lg"
            to="/docs/getting-started/install">
            Install X07
          </Link>
          <Link
            className="button button--outline button--lg"
            to="/mcp">
            MCP + Hardproof
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

function QuickStartSection() {
  return (
    <section className={styles.pathSection}>
      <div className="container">
        <div className={styles.sectionIntro}>
          <p className={styles.sectionEyebrow}>Choose a path</p>
          <h2>Start from the entrypoint that matches the job</h2>
          <p>
            The website serves two audiences on purpose: humans need a clear
            overview, and agents need stable machine-readable contracts.
          </p>
        </div>
        <div className={styles.cardGrid}>
          {quickStartPaths.map((card) => (
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
  );
}

function ComparisonSection() {
  return (
    <section className={styles.comparisonSection}>
      <div className="container">
        <div className={styles.sectionIntro}>
          <p className={styles.sectionEyebrow}>Why X07 is opinionated</p>
          <h2>Less guesswork in the language. Less guesswork in the repair loop.</h2>
          <p>
            The first example is about API ambiguity. The second is about what
            happens when the first attempt is wrong and you need a repair loop a
            human reviewer can follow.
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

function AuthoringSection() {
  return (
    <section className={styles.pathSection}>
      <div className="container">
        <div className={styles.sectionIntro}>
          <p className={styles.sectionEyebrow}>Direct authoring</p>
          <h2>Authoring X07 directly is a bet we test, not assume</h2>
          <p>
            Agents and humans can write X07 directly, and the toolchain now
            backs that path with a text projection, behavioral docs, and
            suggestion-bearing diagnostics.
          </p>
        </div>
        <div className={styles.cardGrid}>
          {authoringFeatures.map((card) => (
            <div key={card.title} className={styles.card}>
              <p className={styles.cardEyebrow}>{card.eyebrow}</p>
              <h3>{card.title}</h3>
              <p>{card.body}</p>
            </div>
          ))}
        </div>
        <p className={styles.sectionNote}>
          Direct authoring is an explicitly gated bet, not a settled
          conclusion. A comparative agent eval in the x07 repo (
          <Link href="https://github.com/x07lang/x07/tree/main/labs/agent-eval">
            labs/agent-eval
          </Link>
          ) decides how much deeper the language investment goes. The results
          will be published either way.
        </p>
      </div>
    </section>
  );
}

export default function Home(): ReactNode {
  return (
    <Layout
      title="x07lang.org"
      description="X07 is the deterministic, certifiable execution substrate for agent-written software: solve worlds, record/replay, budgets, capability sandboxes, structured diagnostics with quickfixes, spec-first testing (XTAL), and proof-backed certification.">
      <HomepageHeader />
      <main>
        <QuickStartSection />
        <ComparisonSection />
        <AuthoringSection />
        <section className={styles.ecosystemSection}>
          <div className="container">
            <div className={styles.sectionIntro}>
              <p className={styles.sectionEyebrow}>Ecosystem</p>
              <h2>A deliberately small active surface</h2>
              <p>
                The 2026-06 scope cut focused the project on five active repos:
                x07, x07-mcp, x07-registry, x07-wasm-backend, and Hardproof.
                Earlier experiments — studio, forge, crewops, tactics, device
                host, web UI, the sentinel reference stack, and the app
                platform — are archived.
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
