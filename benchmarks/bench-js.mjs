import { readFile } from 'node:fs/promises';
import { writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

import { TextMate } from 'textmatelib';
import vscodeTextmate from 'vscode-textmate';
import { createHighlighter } from 'shiki';

const { Registry: VsRegistry, parseRawGrammar, INITIAL } = vscodeTextmate;

import {
  WORKLOADS,
  THEME_NAME,
  BENCH,
  readFixture,
  readGrammar,
  grammarPath,
  readTheme,
  toVscodeTextmateTheme,
  median,
  mbPerSec,
  fmt,
} from './lib/workloads.mjs';

const require = createRequire(import.meta.url);
const __dirname = dirname(fileURLToPath(import.meta.url));

const oniguruma = require('vscode-oniguruma');

// ---------------------------------------------------------------------------
// Engine setup
// ---------------------------------------------------------------------------

async function setupTml() {
  const tm = await TextMate.create();
  tm.setTheme(readTheme(THEME_NAME));
  const grammars = {};
  for (const w of WORKLOADS) {
    grammars[w.scope] = await tm.loadGrammar(w.scope, readGrammar(w.grammar));
    if (!grammars[w.scope]) throw new Error(`tml-js failed to load grammar ${w.grammar}`);
  }
  return { grammars };
}

async function setupVscodeTextmate() {
  const wasmBin = await readFile(require.resolve('vscode-oniguruma/release/onig.wasm'));
  await oniguruma.loadWASM(wasmBin);
  const onigLib = Promise.resolve({
    createOnigScanner: (patterns) => new oniguruma.OnigScanner(patterns),
    createOnigString: (s) => new oniguruma.OnigString(s),
  });

  const scopeToContent = {};
  for (const w of WORKLOADS) scopeToContent[w.scope] = readGrammar(w.grammar);

  const registry = new VsRegistry({
    onigLib,
    loadGrammar: async (scopeName) => {
      const content = scopeToContent[scopeName];
      return content ? parseRawGrammar(content, `${scopeName}.json`) : null;
    },
  });
  registry.setTheme(toVscodeTextmateTheme(readTheme(THEME_NAME)));

  const grammars = {};
  for (const w of WORKLOADS) {
    grammars[w.scope] = await registry.loadGrammar(w.scope);
    if (!grammars[w.scope]) throw new Error(`vscode-textmate failed to load grammar ${w.scope}`);
  }
  return { grammars };
}

async function setupShiki() {
  const hl = await createHighlighter({
    themes: [THEME_NAME],
    langs: WORKLOADS.map((w) => w.grammar),
  });
  return { hl };
}

// ---------------------------------------------------------------------------
// Tokenization passes (one full file each)
// ---------------------------------------------------------------------------

function tmlScopePass(grammar, lines) {
  let state = null;
  let count = 0;
  for (const line of lines) {
    const r = grammar.tokenizeLine(line, state);
    state = r.ruleStack;
    count += r.tokens.length;
  }
  return count;
}

function tmlThemedPass(grammar, lines) {
  let state = null;
  let count = 0;
  for (const line of lines) {
    const r = grammar.tokenizeLine2(line, state);
    state = r.ruleStack;
    count += r.tokens.length / 2;
  }
  return count;
}

// Whole-document API: one JS<->WASM crossing for the entire file (rule stack carried
// inside WASM). This is what a highlighter should use for static text.
function tmlScopeBatchPass(grammar, lines) {
  const linesTokens = grammar.tokenizeLines(lines);
  let count = 0;
  for (const lineTokens of linesTokens) count += lineTokens.length;
  return count;
}

function tmlThemedBatchPass(grammar, lines) {
  const r = grammar.tokenizeLines2(lines);
  return r.tokens.length / 2;
}

function vsScopePass(grammar, lines) {
  let state = INITIAL;
  let count = 0;
  for (const line of lines) {
    const r = grammar.tokenizeLine(line, state);
    state = r.ruleStack;
    count += r.tokens.length;
  }
  return count;
}

function vsThemedPass(grammar, lines) {
  let state = INITIAL;
  let count = 0;
  for (const line of lines) {
    const r = grammar.tokenizeLine2(line, state);
    state = r.ruleStack;
    count += r.tokens.length / 2;
  }
  return count;
}

function shikiThemedPass(hl, content, lang) {
  const r = hl.codeToTokens(content, { lang, theme: THEME_NAME });
  let count = 0;
  for (const row of r.tokens) count += row.length;
  return count;
}

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

function time(passFn) {
  for (let i = 0; i < BENCH.warmup; i++) passFn();
  const samples = [];
  let lastCount = 0;
  for (let i = 0; i < BENCH.runs; i++) {
    const t0 = process.hrtime.bigint();
    lastCount = passFn();
    const t1 = process.hrtime.bigint();
    samples.push(Number(t1 - t0));
  }
  return { ns: median(samples), count: lastCount };
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

console.log(`TextMateLib JS/WASM benchmark — theme: ${THEME_NAME}, warmup ${BENCH.warmup} / runs ${BENCH.runs}`);
console.log(`Node ${process.version} on ${process.platform}/${process.arch}\n`);

const tml = await setupTml();
const vs = await setupVscodeTextmate();
const shiki = await setupShiki();

const scopeRows = [];
const themedRows = [];

for (const w of WORKLOADS) {
  const { content, lines, bytes } = readFixture(w.fixture);
  const mb = bytes / (1024 * 1024);
  process.stdout.write(`Benchmarking ${w.id} (${mb.toFixed(2)} MB, ${lines.length} lines) ... `);

  // --- scope tokenization: tml-js (per-line + batch) vs vscode-textmate ---
  const tmlScope = time(() => tmlScopePass(tml.grammars[w.scope], lines));
  const tmlScopeBatch = time(() => tmlScopeBatchPass(tml.grammars[w.scope], lines));
  const vsScope = time(() => vsScopePass(vs.grammars[w.scope], lines));

  // --- themed tokenization: tml-js (per-line + batch) vs vscode-textmate vs shiki ---
  const tmlThemed = time(() => tmlThemedPass(tml.grammars[w.scope], lines));
  const tmlThemedBatch = time(() => tmlThemedBatchPass(tml.grammars[w.scope], lines));
  const vsThemed = time(() => vsThemedPass(vs.grammars[w.scope], lines));
  const shThemed = time(() => shikiThemedPass(shiki.hl, content, w.grammar));

  scopeRows.push({
    id: w.id,
    mb,
    tml: mbPerSec(bytes, tmlScope.ns),
    tmlBatch: mbPerSec(bytes, tmlScopeBatch.ns),
    vs: mbPerSec(bytes, vsScope.ns),
    tmlTokens: tmlScope.count,
    tmlBatchTokens: tmlScopeBatch.count,
    vsTokens: vsScope.count,
  });
  themedRows.push({
    id: w.id,
    mb,
    tml: mbPerSec(bytes, tmlThemed.ns),
    tmlBatch: mbPerSec(bytes, tmlThemedBatch.ns),
    vs: mbPerSec(bytes, vsThemed.ns),
    shiki: mbPerSec(bytes, shThemed.ns),
    tmlTokens: tmlThemed.count,
    tmlBatchTokens: tmlThemedBatch.count,
  });

  console.log('done');
}

// ---------------------------------------------------------------------------
// Report
// ---------------------------------------------------------------------------

function scopeTable(rows) {
  const lines = [];
  lines.push('| File | Size | tml-js (per-line) | tml-js (batch) | vscode-textmate | Batch speedup |');
  lines.push('|------|------|-------------------|----------------|-----------------|---------------|');
  for (const r of rows) {
    lines.push(
      `| ${r.id} | ${r.mb.toFixed(2)} MB | ${fmt(r.tml)} MB/s | **${fmt(r.tmlBatch)} MB/s** | ${fmt(r.vs)} MB/s | ${fmt(r.tmlBatch / r.vs, 2)}× |`
    );
  }
  return lines.join('\n');
}

function themedTable(rows) {
  const lines = [];
  lines.push('| File | Size | tml-js (per-line) | tml-js (batch) | vscode-textmate | shiki | Batch speedup vs vscode-textmate |');
  lines.push('|------|------|-------------------|----------------|-----------------|-------|----------------------------------|');
  for (const r of rows) {
    lines.push(
      `| ${r.id} | ${r.mb.toFixed(2)} MB | ${fmt(r.tml)} MB/s | **${fmt(r.tmlBatch)} MB/s** | ${fmt(r.vs)} MB/s | ${fmt(r.shiki)} MB/s | ${fmt(r.tmlBatch / r.vs, 2)}× |`
    );
  }
  return lines.join('\n');
}

const scopeMd = scopeTable(scopeRows);
const themedMd = themedTable(themedRows);

console.log('\n### Scope tokenization (scope-name tokens)\n');
console.log(scopeMd);
console.log('\n### Themed tokenization (binary/themed tokens)\n');
console.log(themedMd);

// Correctness guard: token counts should be in the same ballpark.
console.log('\nToken-count sanity (scope mode):');
for (const r of scopeRows) {
  const ratio = r.tmlTokens / r.vsTokens;
  const flag = ratio > 1.25 || ratio < 0.8 ? '  <-- DIVERGENT' : '';
  console.log(`  ${r.id}: tml-js ${r.tmlTokens} vs vscode-textmate ${r.vsTokens} (${ratio.toFixed(2)}×)${flag}`);
}

// Correctness guard: the batch path must produce exactly the same token counts as the
// per-line path (it carries the rule stack internally — any divergence is a bug).
console.log('\nBatch vs per-line token-count parity (must be exact):');
for (const r of scopeRows) {
  const ok = r.tmlBatchTokens === r.tmlTokens;
  console.log(`  ${r.id} scope:  batch ${r.tmlBatchTokens} vs per-line ${r.tmlTokens}  ${ok ? 'OK' : '<-- MISMATCH'}`);
}
for (const r of themedRows) {
  const ok = r.tmlBatchTokens === r.tmlTokens;
  console.log(`  ${r.id} themed: batch ${r.tmlBatchTokens} vs per-line ${r.tmlTokens}  ${ok ? 'OK' : '<-- MISMATCH'}`);
}

const out = [
  `<!-- generated by benchmarks/bench-js.mjs — Node ${process.version} on ${process.platform}/${process.arch} -->`,
  `Theme: \`${THEME_NAME}\` · median of ${BENCH.runs} runs (${BENCH.warmup} warmup)`,
  '',
  '### Scope tokenization',
  '',
  scopeMd,
  '',
  '### Themed tokenization',
  '',
  themedMd,
  '',
].join('\n');
writeFileSync(resolve(__dirname, 'results.md'), out, 'utf-8');
console.log('\nWrote benchmarks/results.md');
