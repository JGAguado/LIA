// @ts-check
import { cpSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import sitemap from '@astrojs/sitemap';
import mdx from '@astrojs/mdx';
import tailwindcss from '@tailwindcss/vite';
import mermaid from 'astro-mermaid';
import remarkMath from 'remark-math';
import rehypeKatex from 'rehype-katex';
import starlightImageZoom from 'starlight-image-zoom';

const repoRoot = fileURLToPath(new URL('..', import.meta.url));
const publicDir = fileURLToPath(new URL('./public/generated/', import.meta.url));

/**
 * The interactive BOM and STL exports are regenerated straight from KiCad/CAD
 * tools into hardware/ and enclosure/. Browsers can't reliably fetch those
 * files across the project root via Vite's dev-only `/@fs/` paths (subframe
 * and cross-context requests to `/@fs/` are rejected by Vite's dev-server
 * security checks), so this mirrors them into public/generated/ on every
 * `astro dev` / `astro build` start — no manual copy step, always current.
 */
function syncGeneratedAssets() {
  return {
    name: 'lia-sync-generated-assets',
    hooks: {
      'astro:config:setup'() {
        cpSync(`${repoRoot}hardware/pcb/bom`, `${publicDir}bom`, { recursive: true, force: true });
        const enclosureVersions = {
          'V1.0-Detachable': 'v1.0-detachable',
          'V1.0-Fixed': 'v1.0-fixed',
          'V1.1-Fixed': 'v1.1-fixed',
        };
        for (const [folder, slug] of Object.entries(enclosureVersions)) {
          cpSync(`${repoRoot}enclosure/${folder}/stl`, `${publicDir}stl/${slug}`, {
            recursive: true,
            force: true,
          });
        }
      },
    },
  };
}

// https://astro.build/config
export default defineConfig({
  site: 'https://jgaguado.github.io',
  base: '/LIA',
  markdown: {
    // Admonitions use Starlight's own native `:::note`/`:::tip`/`:::caution`/
    // `:::danger` directive syntax (handled internally by Starlight's own
    // remark-directive-based Aside rendering) -- an earlier attempt used
    // GitHub-style `> [!NOTE]` blockquotes via a third-party remark plugin,
    // but that plugin produces its own generic, unstyled `.admonition` markup
    // rather than Starlight's `<aside class="starlight-aside--*">`, which is
    // why they rendered as plain text.
    remarkPlugins: [remarkMath],
    rehypePlugins: [rehypeKatex],
  },
  integrations: [
    syncGeneratedAssets(),
    sitemap(),
    mermaid({
      theme: 'base',
      autoTheme: true,
    }),
    starlight({
      title: 'LIA',
      description: 'Off-grid. On track. An open-source, Meshtastic-powered pet tracker.',
      logo: {
        light: './src/assets/brand/Primary light.png',
        dark: './src/assets/brand/Primary dark.png',
        replacesTitle: true,
      },
      favicon: '/favicon.png',
      social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/JGAguado/LIA' }],
      editLink: {
        baseUrl: 'https://github.com/JGAguado/LIA/edit/main/website/',
      },
      customCss: ['./src/styles/global.css'],
      plugins: [starlightImageZoom()],
      sidebar: [
        { label: 'Introduction', link: '/docs/' },
        { label: 'Getting Started', link: '/docs/getting-started/' },
        {
          label: 'Hardware',
          items: [
            { label: 'PCB', link: '/docs/hardware/pcb/' },
            { label: 'Schematics', link: '/docs/hardware/schematics/' },
            { label: 'BOM', link: '/docs/hardware/bom/' },
            { label: 'Assembly', link: '/docs/hardware/assembly/' },
            { label: 'Enclosure', link: '/docs/hardware/enclosure/' },
          ],
        },
        {
          label: 'Firmware',
          items: [
            { label: 'Meshtastic Configuration', link: '/docs/firmware/meshtastic-configuration/' },
            { label: 'Building the Firmware', link: '/docs/firmware/building-the-firmware/' },
            { label: 'Microcontroller', link: '/docs/firmware/microcontroller/' },
            { label: 'Power Management', link: '/docs/firmware/power-management/' },
            { label: 'Battery', link: '/docs/firmware/battery/' },
            { label: 'Accelerometer', link: '/docs/firmware/accelerometer/' },
            { label: 'GPS', link: '/docs/firmware/gps/' },
            { label: 'LoRa', link: '/docs/firmware/lora/' },
            { label: 'RGB LED', link: '/docs/firmware/rgb-led/' },
          ],
        },
        { label: 'Development History', link: '/docs/development-history/' },
        { label: 'Manufacturing', link: '/docs/manufacturing/' },
        { label: 'Troubleshooting', link: '/docs/troubleshooting/' },
        { label: 'FAQ', link: '/docs/faq/' },
        { label: 'Downloads', link: '/docs/downloads/' },
        { label: 'License', link: '/docs/license/' },
        { label: 'Contributing', link: '/docs/contributing/' },
      ],
    }),
    mdx(),
  ],
  vite: {
    plugins: [tailwindcss()],
    server: {
      // Allow importing renders/CAD exports directly from their source
      // folders (hardware/, enclosure/) outside website/, so updating a
      // render or STL file doesn't require copying it into the website too.
      fs: { allow: ['..'] },
    },
  },
});
