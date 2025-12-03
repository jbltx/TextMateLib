import js from '@eslint/js';
import typescript from '@typescript-eslint/eslint-plugin';
import tsParser from '@typescript-eslint/parser';
import prettier from 'eslint-plugin-prettier';

export default [
  {
    ignores: [
      'node_modules/**',
      '**/node_modules/**',
      'dist/**',
      '**/dist/**',
      'build/**',
      '**/build/**',
      'build-*/**',
      'coverage/**',
      '.changeset/**',
      'docs/**',
      'wasm/**',
      '**/wasm/**',
      '*.min.js',
      '*.min.css',
      '*.wasm',
      '*.map',
      '*.d.ts', // Ignore generated TypeScript declarations
      '.DS_Store',
      '.vscode/**',
      '.idea/**',
      'thirdparty/**',
      'src/csharp/**',
      'src/**/*.cpp',
      'src/**/*.h',
      'examples/**',
      'playground/**',
      'tests/**/*.cpp',
      'tests/csharp/**',
    ],
  },
  {
    files: ['packages/**/*.{js,ts,tsx}', '*.{js,ts,tsx}'],
    languageOptions: {
      parser: tsParser,
      parserOptions: {
        ecmaVersion: 2020,
        sourceType: 'module',
      },
      globals: {
        console: 'readonly',
        process: 'readonly',
        Buffer: 'readonly',
        __dirname: 'readonly',
        __filename: 'readonly',
      },
    },
    plugins: {
      '@typescript-eslint': typescript,
      prettier: prettier,
    },
    rules: {
      ...js.configs.recommended.rules,
      ...typescript.configs.recommended.rules,
      'prettier/prettier': 'warn',
      '@typescript-eslint/no-unused-vars': [
        'error',
        {
          argsIgnorePattern: '^_',
        },
      ],
      '@typescript-eslint/explicit-module-boundary-types': 'off',
      '@typescript-eslint/no-explicit-any': 'warn',
      'no-console': 'off',
    },
  },
];
