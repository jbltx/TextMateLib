import { defineConfig } from 'vite';
import { resolve } from 'path';
import { viteStaticCopy } from 'vite-plugin-static-copy';

export default defineConfig({
  // Set base path for GitHub Pages deployment
  // Use '/' for local dev, '/TextMateLib/' for GitHub Pages
  base: process.env.GITHUB_ACTIONS ? '/TextMateLib/' : '/',

  server: {
    // Allow serving files from parent directory
    fs: {
      allow: ['..']
    }
  },
  // Create an alias for the grammars/themes directory - only .json files
  resolve: {
    alias: [
      {
        find: /^\/tm-themes\/(.+\.json)$/,
        replacement: resolve(__dirname, '../thirdparty/textmate-grammars-themes/packages/tm-themes/themes/$1')
      },
      {
        find: /^\/tm-grammars\/(.+\.json)$/,
        replacement: resolve(__dirname, '../thirdparty/textmate-grammars-themes/packages/tm-grammars/grammars/$1')
      }
    ]
  },
  plugins: [
    viteStaticCopy({
      targets: [
        {
          src: '../thirdparty/textmate-grammars-themes/packages/tm-themes/themes/*.json',
          dest: 'tm-themes'
        },
        {
          src: '../thirdparty/textmate-grammars-themes/packages/tm-grammars/grammars/*.json',
          dest: 'tm-grammars'
        },
        {
          // Copy Doxygen documentation to build output
          // Docs are generated in public/docs during postinstall
          src: 'public/docs/**/*',
          dest: 'docs'
        }
      ]
    })
  ]
});
