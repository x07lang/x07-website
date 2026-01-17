import {themes as prismThemes} from 'prism-react-renderer';
import type {Config} from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';
import {createRedirects} from './redirects.generated';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

const config: Config = {
  title: 'X07',
  tagline: 'Agent-first systems language',
  favicon: 'img/favicon.svg',

  // Future flags, see https://docusaurus.io/docs/api/docusaurus-config#future
  future: {
    v4: true, // Improve compatibility with the upcoming Docusaurus v4
  },

  // Set the production url of your site here
  url: 'https://x07lang.org',
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: '/',

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: 'x07lang', // Usually your GitHub org/user name.
  projectName: 'x07lang.org', // Usually your repo name.

  onBrokenLinks: 'throw',

  markdown: {
    hooks: {
      onBrokenMarkdownLinks: 'throw',
    },
  },

  // Even if you don't use internationalization, you can use this field to set
  // useful metadata like html lang. For example, if your site is Chinese, you
  // may want to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          path: './docs',
          routeBasePath: 'docs',
          sidebarPath: './sidebars.generated.ts',
          exclude: ['**/SUMMARY.md'],
          lastVersion: 'current',
          versions: {
            current: {
              label: 'latest',
              path: '',
            },
          },
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      } satisfies Preset.Options,
    ],
  ],

  plugins: [
    [
      '@docusaurus/plugin-client-redirects',
      {
        createRedirects,
      },
    ],
  ],

  themeConfig: {
    // Replace with your project's social card
    image: 'img/social-card.svg',
    announcementBar: {
      id: 'active_development',
      content:
        'X07 is under active development. APIs and tooling may change.',
      backgroundColor: '#25c2a0',
      textColor: '#1b1b1d',
      isCloseable: true,
    },
    colorMode: {
      respectPrefersColorScheme: true,
    },
    navbar: {
      title: 'X07',
      logo: {
        alt: 'X07 Logo',
        src: 'img/logo-icon-light.png',
        srcDark: 'img/logo-icon-dark.png',
      },
      items: [
        {
          to: '/docs/',
          position: 'left',
          label: 'Docs',
        },
        {type: 'docsVersionDropdown', position: 'left'},
        {
          to: '/agent',
          label: 'Agent',
          position: 'left',
        },
        {
          href: 'https://github.com/x07lang/x07',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Docs',
          items: [
            {label: 'Getting started', to: '/docs/getting-started/'},
            {label: 'Language', to: '/docs/language/'},
            {label: 'Toolchain', to: '/docs/toolchain/'},
            {label: 'Guides', to: '/docs/guides/'},
          ],
        },
        {
          title: 'Agent',
          items: [
            {label: 'Agent portal', to: '/agent'},
            {label: 'Latest manifest', href: '/agent/latest/manifest.json'},
            {label: 'Schemas', href: '/agent/latest/schemas/x07ast.schema.json'},
          ],
        },
        {
          title: 'Community',
          items: [
            {label: 'GitHub', href: 'https://github.com/x07lang/x07'},
            {label: 'RFCs', href: 'https://github.com/x07lang/x07-rfcs'},
            {label: 'Contributing', to: '/docs/contributing'},
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} x07lang.org`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.dracula,
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
