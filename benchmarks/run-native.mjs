import { existsSync } from 'node:fs';
import { execFileSync, execSync } from 'node:child_process';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

import { WORKLOADS, fixturePath, grammarPath, themePath } from './lib/workloads.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(__dirname, '..');

function firstExisting(paths) {
  return paths.map((p) => resolve(repoRoot, p)).find((p) => existsSync(p));
}

function locate() {
  // Prefer the canonical build:native output (packages/tml-cpp/build) over any older
  // top-level build/ so the native reference reflects current, optimized sources.
  const tmlLib = firstExisting(['packages/tml-cpp/build/libtml.a', 'build/libtml.a']);
  const onigLib = firstExisting([
    'packages/tml-cpp/build/oniguruma/lib/libonig.a',
    'packages/tml-cpp/build/thirdparty/oniguruma/build/libonig.a',
    'build/oniguruma/lib/libonig.a',
    'build/thirdparty/oniguruma/build/libonig.a',
  ]);
  const onigInc = firstExisting([
    'packages/tml-cpp/build/oniguruma/include',
    'build/oniguruma/include',
  ]);
  return { tmlLib, onigLib, onigInc };
}

function buildNative() {
  console.error('Native libraries not found or stale — building (npm run build:native)...');
  execSync('npm run build:native', { cwd: repoRoot, stdio: 'inherit' });
}

let { tmlLib, onigLib, onigInc } = locate();
if (!tmlLib || !onigLib || !onigInc) {
  buildNative();
  ({ tmlLib, onigLib, onigInc } = locate());
  if (!tmlLib || !onigLib || !onigInc) {
    console.error('Still could not locate built libraries. Aborting native benchmark.');
    process.exit(1);
  }
}

const srcInc = resolve(repoRoot, 'packages/tml-cpp/src');
const bin = resolve(__dirname, 'bench-native');
const cpp = resolve(__dirname, 'bench-native.cpp');

function compile() {
  const args = [
    '-std=c++17', '-O2',
    `-I${srcInc}`, `-I${onigInc}`,
    cpp, tmlLib, onigLib,
    '-o', bin,
  ];
  console.error(`Compiling: clang++ ${args.join(' ')}`);
  execFileSync('clang++', args, { stdio: 'inherit' });
}

try {
  compile();
} catch (err) {
  console.error('Compile/link failed with prebuilt lib; rebuilding native and retrying...');
  buildNative();
  ({ tmlLib, onigLib, onigInc } = locate());
  compile();
}

const args = ['--theme', themePath()];
for (const w of WORKLOADS) {
  if (!existsSync(fixturePath(w.fixture))) {
    console.error(`Fixture missing: ${w.fixture}. Run \`npm run fixtures\` first.`);
    process.exit(1);
  }
  args.push(w.scope, grammarPath(w.grammar), fixturePath(w.fixture));
}

console.error('\nRunning native reference benchmark...\n');
const out = execFileSync(bin, args, { encoding: 'utf-8' });
console.log(out);
