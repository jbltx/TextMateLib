import { defineConfig } from 'vite';
import { resolve } from 'path';
import { viteStaticCopy } from 'vite-plugin-static-copy';
import { createReadStream, existsSync } from 'fs';

// Custom plugin to serve grammar/theme files during dev
function serveGrammarsThemes() {
  const themesDir = resolve(__dirname, '../../thirdparty/textmate-grammars-themes/packages/tm-themes/themes');
  const grammarsDir = resolve(__dirname, '../../thirdparty/textmate-grammars-themes/packages/tm-grammars/grammars');

  return {
    name: 'serve-grammars-themes',
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        if (req.url?.startsWith('/tm-themes/')) {
          const fileName = req.url.replace('/tm-themes/', '');
          const filePath = resolve(themesDir, fileName);
          if (existsSync(filePath)) {
            res.setHeader('Content-Type', 'application/json');
            createReadStream(filePath).pipe(res);
            return;
          }
        }
        if (req.url?.startsWith('/tm-grammars/')) {
          const fileName = req.url.replace('/tm-grammars/', '');
          const filePath = resolve(grammarsDir, fileName);
          if (existsSync(filePath)) {
            res.setHeader('Content-Type', 'application/json');
            createReadStream(filePath).pipe(res);
            return;
          }
        }
        next();
      });
    }
  };
}

export default defineConfig({
  // Base path for deployment
  // Custom domain (tml.jbltx.com) serves from '/', so always use '/'
  base: '/',

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
        replacement: resolve(__dirname, '../../thirdparty/textmate-grammars-themes/packages/tm-themes/themes/$1')
      },
      {
        find: /^\/tm-grammars\/(.+\.json)$/,
        replacement: resolve(__dirname, '../../thirdparty/textmate-grammars-themes/packages/tm-grammars/grammars/$1')
      }
    ]
  },
  plugins: [
    // Serve grammars/themes during dev (configureServer only runs in dev mode)
    serveGrammarsThemes(),
    viteStaticCopy({
      targets: [
        {
          src: '../../thirdparty/textmate-grammars-themes/packages/tm-themes/themes/*.json',
          dest: 'tm-themes'
        },
        {
          src: '../../thirdparty/textmate-grammars-themes/packages/tm-grammars/grammars/*.json',
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
