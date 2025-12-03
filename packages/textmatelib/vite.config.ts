import { defineConfig } from 'vite';
import dts from 'vite-plugin-dts';
import { resolve } from 'path';
import fs from 'fs';

export default defineConfig({
  build: {
    lib: {
      entry: {
        index: resolve(__dirname, 'src/index.ts'),
        'low-level': resolve(__dirname, 'src/low-level.ts'),
        'high-level': resolve(__dirname, 'src/high-level.ts'),
      },
      name: 'TextMateLib',
      fileName: (format, entryName) => {
        if (format === 'cjs') {
          return `${entryName}.cjs`;
        }
        return `${entryName}.mjs`;
      },
    },
    rollupOptions: {
      // No external dependencies - bundle everything
      external: [],
      output: [
        // CommonJS build
        {
          format: 'cjs',
          dir: 'dist',
          entryFileNames: '[name].cjs',
        },
        // ES Module build
        {
          format: 'es',
          dir: 'dist',
          entryFileNames: '[name].mjs',
        },
      ],
    },
    sourcemap: true,
    minify: 'terser',
  },

  plugins: [
    // Generate TypeScript definitions
    dts({
      insertTypesEntry: false,
      include: ['src'],
      outDir: 'dist',
    }),

    // Custom plugin to copy WASM files to dist during build
    {
      name: 'copy-wasm',
      apply: 'build',
      enforce: 'post',
      async generateBundle() {
        const wasmDir = resolve(__dirname, 'wasm');
        const distDir = resolve(__dirname, 'dist');

        // Ensure dist/wasm directory exists
        const distWasmDir = `${distDir}/wasm`;
        if (!fs.existsSync(distWasmDir)) {
          fs.mkdirSync(distWasmDir, { recursive: true });
        }

        // Copy WASM files
        const wasmFiles = fs.readdirSync(wasmDir);
        for (const file of wasmFiles) {
          const src = `${wasmDir}/${file}`;

          const data = fs.readFileSync(src);
          this.emitFile({
            fileName: `wasm/${file}`,
            source: data,
            type: 'asset',
          });
        }
      },
    },
  ],
});
