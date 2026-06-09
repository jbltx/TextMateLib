import { readFileSync, existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const root = resolve(__dirname, '..');

// tm-grammars / tm-themes are devDependencies of the tml-js package and are the same
// grammar/theme source shiki uses, guaranteeing grammar parity across all engines.
const tmGrammarsDir = resolve(
  root,
  '../packages/tml-js/node_modules/tm-grammars/grammars'
);
const tmThemesDir = resolve(root, '../packages/tml-js/node_modules/tm-themes/themes');

export const THEME_NAME = 'github-dark';

// { id, fixture file (in fixtures/), grammar (tm-grammars name), scope }
export const WORKLOADS = [
  { id: 'TypeScript (vscode.d.ts)', fixture: 'vscode.d.ts', grammar: 'typescript', scope: 'source.ts' },
  { id: 'JavaScript (jQuery)', fixture: 'jquery.js', grammar: 'javascript', scope: 'source.js' },
  { id: 'CSS (Bootstrap)', fixture: 'bootstrap.css', grammar: 'css', scope: 'source.css' },
  { id: 'Python (typing.py)', fixture: 'typing.py', grammar: 'python', scope: 'source.python' },
];

export function fixturePath(name) {
  return resolve(root, 'fixtures', name);
}

export function grammarPath(name) {
  return resolve(tmGrammarsDir, `${name}.json`);
}

export function readGrammar(name) {
  return readFileSync(grammarPath(name), 'utf-8');
}

export function themePath(name = THEME_NAME) {
  return resolve(tmThemesDir, `${name}.json`);
}

export function readTheme(name = THEME_NAME) {
  return readFileSync(themePath(name), 'utf-8');
}

export function readFixture(name) {
  const p = fixturePath(name);
  if (!existsSync(p)) {
    throw new Error(
      `Fixture "${name}" not found. Run \`npm run fixtures\` first (node prepare-fixtures.mjs).`
    );
  }
  const content = readFileSync(p, 'utf-8');
  return { content, lines: content.split('\n'), bytes: Buffer.byteLength(content, 'utf-8') };
}

// vscode-textmate's setTheme wants { settings: [...] }; tm-themes expose `tokenColors`.
export function toVscodeTextmateTheme(themeJson) {
  const t = JSON.parse(themeJson);
  return { name: t.name, settings: t.tokenColors ?? t.settings ?? [] };
}

export const BENCH = {
  warmup: 3,
  runs: 7,
};

export function median(nums) {
  const s = [...nums].sort((a, b) => a - b);
  const mid = Math.floor(s.length / 2);
  return s.length % 2 ? s[mid] : (s[mid - 1] + s[mid]) / 2;
}

// Median nanoseconds -> MB/s for a given byte count.
export function mbPerSec(bytes, nsMedian) {
  const seconds = nsMedian / 1e9;
  return bytes / (1024 * 1024) / seconds;
}

export function fmt(n, digits = 1) {
  return n.toLocaleString('en-US', { minimumFractionDigits: digits, maximumFractionDigits: digits });
}
