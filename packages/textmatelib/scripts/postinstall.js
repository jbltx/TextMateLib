#!/usr/bin/env node

/**
 * Post-install script for textmatelib
 * Verifies that WASM binaries are present and valid
 */

import fs from 'fs';
import path from 'path';

const packageRoot = path.resolve(__dirname, '..');
const wasmDir = path.join(packageRoot, 'wasm');
const distWasmDir = path.join(packageRoot, 'dist', 'wasm');

const requiredFiles = ['tml-standard.js', 'tml-standard.wasm'];
const errors = [];

console.log('[TextMateLib] Verifying WASM binaries...');

// Check if WASM files exist in the package
for (const file of requiredFiles) {
  const wasmPath = path.join(wasmDir, file);

  if (!fs.existsSync(wasmPath)) {
    errors.push(`Missing WASM file: ${file}`);
  } else {
    const stats = fs.statSync(wasmPath);
    console.log(`✓ Found ${file} (${(stats.size / 1024).toFixed(1)} KB)`);
  }
}

// If in development (dist/ doesn't exist yet), skip dist check
if (fs.existsSync(distWasmDir)) {
  for (const file of requiredFiles) {
    const distPath = path.join(distWasmDir, file);

    if (!fs.existsSync(distPath)) {
      console.warn(`⚠ Warning: ${file} not found in dist/wasm/`);
      console.warn('  This is normal during development. Run "npm run build" to generate dist/');
    } else {
      const stats = fs.statSync(distPath);
      console.log(`✓ Found dist/wasm/${file} (${(stats.size / 1024).toFixed(1)} KB)`);
    }
  }
}

// Report errors
if (errors.length > 0) {
  console.error('\n❌ WASM setup failed:');
  errors.forEach((err) => console.error(`  - ${err}`));
  console.error('\nThe npm package may be corrupted. Please try:');
  console.error('  1. Delete node_modules and package-lock.json');
  console.error('  2. Run "npm install" again');
  process.exit(1);
}

console.log('\n✓ WASM binaries verified successfully!');
console.log('  TextMateLib is ready to use.');
