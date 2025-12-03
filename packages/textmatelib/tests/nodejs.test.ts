/**
 * Test WASM module in Node.js environment
 * Verifies that the package works as a native library in Node.js
 */

import { describe, it, expect } from 'vitest';
import { loadTextMateModule, resetModule } from '../src/loader';

describe('Node.js Environment Tests', () => {
  it('should be able to load WASM module in Node.js', async () => {
    // Reset module state
    resetModule();

    // Try to load with debug enabled to see what's happening
    const module = await loadTextMateModule({
      debug: true,
    });

    expect(module).toBeDefined();
    expect(module.Registry).toBeDefined();
    expect(module.Grammar).toBeDefined();
  });

  it('should create Registry instance in Node.js', async () => {
    const module = await loadTextMateModule();

    // Try to instantiate a Registry
    const registry = new module.Registry();
    expect(registry).toBeDefined();
  });

  it('should be usable with file paths in Node.js', async () => {
    // This test verifies that users can load grammars from files
    // without needing to manually read them first (when integrated with file I/O)
    const module = await loadTextMateModule();
    const registry = new module.Registry();

    expect(typeof registry.loadGrammarFromContent).toBe('function');
    expect(typeof registry.setTheme).toBe('function');
  });
});

describe('Node.js vs Browser Compatibility', () => {
  it('should detect Node.js environment correctly', () => {
    // This is a meta test - just verify we're in Node.js
    expect(typeof window).toBe('undefined');
    expect(typeof process).toBe('object');
    expect(typeof require).not.toBe('undefined');
  });

  it('should export the same APIs in Node.js as in browser', async () => {
    const module = await loadTextMateModule();

    // These should exist in both environments
    expect(module.Registry).toBeDefined();
    expect(module.Grammar).toBeDefined();
    expect(typeof module.Registry).toBe('function');
    expect(typeof module.Grammar).toBe('function');
  });
});
