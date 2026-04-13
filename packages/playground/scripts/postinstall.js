#!/usr/bin/env node

// Build WASM and Doxygen documentation
import { execSync } from 'child_process';
import { platform } from 'os';
import { resolve } from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';
import { cpSync, rmSync, existsSync } from 'fs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const projectRoot = resolve(__dirname, '..');
const currentPlatform = platform();

// Build WASM on Linux/macOS
if (currentPlatform === 'linux' || currentPlatform === 'darwin') {
  console.log('🔨 Building WASM module on', currentPlatform);
  try {
    execSync('./build-wasm.sh', { stdio: 'inherit', cwd: projectRoot });
  } catch (error) {
    console.warn('⚠️  Warning: build-wasm.sh failed. WASM module may not be available.');
    console.warn('You can manually build it later by running: ./build-wasm.sh');
  }
} else {
  console.log('⏭️  Skipping WASM build on', currentPlatform);
  console.log('Note: WASM build is only supported on Linux/macOS.');
  console.log('The playground will run in demo mode without real syntax highlighting.');
}

// Build Doxygen documentation
console.log('\n📚 Building Doxygen documentation...');
try {
  const docsDir = resolve(projectRoot, '..', 'tml-cpp', 'docs');
  execSync('doxygen Doxyfile', { stdio: 'inherit', cwd: docsDir });

  // Copy generated docs to playground/public/docs
  const publicDocsDir = resolve(projectRoot, 'public', 'docs');
  const generatedDocsDir = resolve(docsDir, 'html');

  if (existsSync(publicDocsDir)) {
    console.log('📝 Removing existing docs...');
    rmSync(publicDocsDir, { recursive: true, force: true });
  }

  console.log('📋 Copying generated documentation to public/docs...');
  cpSync(generatedDocsDir, publicDocsDir, { recursive: true });
  console.log('✅ Documentation ready at playground/public/docs');

} catch (error) {
  console.warn('⚠️  Warning: Doxygen documentation build failed.');
  console.warn('Make sure Doxygen is installed: brew install doxygen');
  console.warn('You can manually build docs later by running: cd docs && doxygen Doxyfile');
}
