import { mkdirSync, writeFileSync, existsSync, readFileSync, statSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const fixturesDir = resolve(__dirname, 'fixtures');
mkdirSync(fixturesDir, { recursive: true });

// Real-world files used as benchmark workloads. Each has a fallback that synthesizes a
// large file from code already in the repo, so the benchmark runs even offline.
const FIXTURES = [
  {
    name: 'vscode.d.ts',
    url: 'https://raw.githubusercontent.com/microsoft/vscode/main/src/vscode-dts/vscode.d.ts',
    fallback: () => synthesizeFromRepo('packages/tml-js/src/types.ts', 400, '// fallback TS workload\n'),
  },
  {
    name: 'jquery.js',
    url: 'https://code.jquery.com/jquery-3.7.1.js',
    fallback: () => synthesizeFromRepo('packages/tml-js/dist/index.js', 400, '// fallback JS workload\n'),
  },
  {
    name: 'bootstrap.css',
    url: 'https://raw.githubusercontent.com/twbs/bootstrap/v5.3.3/dist/css/bootstrap.css',
    fallback: () => synthesizeCss(6000),
  },
  {
    name: 'typing.py',
    url: 'https://raw.githubusercontent.com/python/cpython/main/Lib/typing.py',
    fallback: () => synthesizePython(4000),
  },
];

function synthesizeFromRepo(relPath, repeats, header) {
  const p = resolve(__dirname, '..', relPath);
  let base = existsSync(p) ? readFileSync(p, 'utf-8') : '';
  if (!base.trim()) base = 'const x = 42;\nfunction f(a, b) { return a + b; }\n';
  let out = header;
  for (let i = 0; i < repeats; i++) out += base + '\n';
  return out;
}

function synthesizeCss(rules) {
  let out = '/* fallback CSS workload */\n';
  for (let i = 0; i < rules; i++) {
    out += `.cls-${i} { color: #${(i % 256).toString(16).padStart(2, '0')}aabb; margin: ${i % 20}px; display: flex; }\n`;
  }
  return out;
}

function synthesizePython(funcs) {
  let out = '# fallback Python workload\nimport os\nfrom typing import List, Dict\n\n';
  for (let i = 0; i < funcs; i++) {
    out += `def func_${i}(a: int, b: str = "x") -> Dict[str, int]:\n    """Docstring ${i}."""\n    return {b: a + ${i}}\n\n`;
  }
  return out;
}

async function fetchText(url, timeoutMs = 20000) {
  const ctrl = new AbortController();
  const t = setTimeout(() => ctrl.abort(), timeoutMs);
  try {
    const res = await fetch(url, { signal: ctrl.signal });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    return await res.text();
  } finally {
    clearTimeout(t);
  }
}

for (const fx of FIXTURES) {
  const dest = resolve(fixturesDir, fx.name);
  if (existsSync(dest) && statSync(dest).size > 0) {
    console.log(`✓ ${fx.name} already present (${(statSync(dest).size / 1024).toFixed(0)} KB)`);
    continue;
  }
  try {
    process.stdout.write(`↓ downloading ${fx.name} ... `);
    const text = await fetchText(fx.url);
    writeFileSync(dest, text, 'utf-8');
    console.log(`ok (${(Buffer.byteLength(text) / 1024).toFixed(0)} KB)`);
  } catch (err) {
    const text = fx.fallback();
    writeFileSync(dest, text, 'utf-8');
    console.log(`download failed (${err.message}); wrote synthetic fallback (${(Buffer.byteLength(text) / 1024).toFixed(0)} KB)`);
  }
}

console.log('\nFixtures ready in benchmarks/fixtures/');
