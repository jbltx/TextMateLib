#!/usr/bin/env node

// Only run build-wasm.sh on Linux/macOS
import { execSync } from 'child_process';
import { platform } from 'os';

const currentPlatform = platform();

if (currentPlatform === 'linux' || currentPlatform === 'darwin') {
  console.log('Running build-wasm.sh on', currentPlatform);
  try {
    execSync('./build-wasm.sh', { stdio: 'inherit', cwd: process.cwd() });
  } catch (error) {
    console.warn('Warning: build-wasm.sh failed. WASM module may not be available.');
    console.warn('You can manually build it later by running: ./build-wasm.sh');
    process.exit(0); // Don't fail the install
  }
} else {
  console.log('Skipping WASM build on', currentPlatform);
  console.log('Note: WASM build is only supported on Linux/macOS.');
  console.log('The playground will run in demo mode without real syntax highlighting.');
}
