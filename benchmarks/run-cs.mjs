import { existsSync, readdirSync } from 'node:fs';
import { execFileSync, execSync } from 'node:child_process';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

import { WORKLOADS, fixturePath, grammarPath, themePath } from './lib/workloads.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(__dirname, '..');
const cppDir = resolve(repoRoot, 'packages/tml-cpp');
const sharedDir = resolve(cppDir, 'build-shared');

// dotnet is required for the C# binding benchmark; skip gracefully if absent.
try {
  execFileSync('dotnet', ['--version'], { stdio: 'ignore' });
} catch {
  console.error('dotnet SDK not found — skipping C# (.NET) benchmark.');
  process.exit(0);
}

// The managed binding P/Invokes a native shared library. Build it if missing.
const haveShared = ['libtml.dylib', 'libtml.so', 'tml.dll', 'Release/tml.dll'].some((f) =>
  existsSync(resolve(sharedDir, f))
);
if (!haveShared) {
  console.error('Native shared library not found — building (scripts/build-shared.sh)...');
  execSync('bash scripts/build-shared.sh', { cwd: cppDir, stdio: ['ignore', 2, 2] });
}

const proj = resolve(__dirname, 'csharp');

console.error('\nBuilding C# (.NET P/Invoke) benchmark...');
execFileSync('dotnet', ['build', '-c', 'Release', '--nologo', '-v', 'q', proj], {
  stdio: ['ignore', 2, 2],
});

const outDir = resolve(proj, 'bin/Release');
const tfm = existsSync(outDir) ? readdirSync(outDir).find((d) => d.startsWith('net')) : undefined;
const dll = tfm ? resolve(outDir, tfm, 'BenchCs.dll') : undefined;
if (!dll || !existsSync(dll)) {
  console.error('Could not locate built BenchCs.dll. Aborting C# benchmark.');
  process.exit(1);
}

const args = ['--theme', themePath()];
for (const w of WORKLOADS) {
  if (!existsSync(fixturePath(w.fixture))) {
    console.error(`Fixture missing: ${w.fixture}. Run \`npm run fixtures\` first.`);
    process.exit(1);
  }
  args.push(w.scope, grammarPath(w.grammar), fixturePath(w.fixture));
}

console.error('\nRunning C# (.NET P/Invoke) benchmark...\n');
const out = execFileSync('dotnet', [dll, ...args], {
  encoding: 'utf-8',
  stdio: ['ignore', 'pipe', 'inherit'],
});
console.log(out);
