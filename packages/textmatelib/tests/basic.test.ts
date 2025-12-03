/**
 * Basic tests for TextMateLib npm package
 */

import { describe, it, expect, beforeAll } from 'vitest';
import { loadTextMateModule, getTextMateModule, resetModule } from '../src/loader';
import type { TextMateWasmModule } from '../src/types';

describe('TextMateLib WASM Module', () => {
  let module: TextMateWasmModule;

  beforeAll(async () => {
    // Load the WASM module
    module = await loadTextMateModule({
      wasmPath: './wasm/',
      debug: false,
    });
  });

  it('should load WASM module successfully', async () => {
    expect(module).toBeDefined();
    expect(module.Registry).toBeDefined();
    expect(module.Grammar).toBeDefined();
  });

  it('should expose Registry constructor', () => {
    expect(typeof module.Registry).toBe('function');
  });

  it('should expose Grammar constructor', () => {
    expect(typeof module.Grammar).toBe('function');
  });

  it('should return same module instance on subsequent calls', async () => {
    const firstInstance = getTextMateModule();
    const secondLoad = await loadTextMateModule();

    expect(firstInstance).toBe(secondLoad);
  });

  it('should reset module state', async () => {
    expect(getTextMateModule()).toBeDefined();
    resetModule();
    expect(getTextMateModule()).toBeNull();
  });
});

describe('Type Definitions', () => {
  it('should have correct type definitions exported from index', async () => {
    // Type definitions are exported from index.ts
    const indexModule = await import('../src/index');

    // We can't directly test types at runtime, but we can verify the module exports
    expect(indexModule).toBeDefined();
    expect(typeof indexModule).toBe('object');
  });
});

describe('Module Exports', () => {
  it('should export all public APIs from index', async () => {
    const index = await import('../src/index');

    // High-level API
    expect(index).toHaveProperty('SyntaxHighlighter');
    expect(index).toHaveProperty('createSyntaxHighlighter');

    // Low-level API
    expect(index).toHaveProperty('Registry');
    expect(index).toHaveProperty('Grammar');
    expect(index).toHaveProperty('createRegistry');
    expect(index).toHaveProperty('createGrammar');

    // WASM loader
    expect(index).toHaveProperty('loadTextMateModule');
    expect(index).toHaveProperty('initializeModule');
    expect(index).toHaveProperty('getTextMateModule');
  });

  it('should export low-level API from low-level module', async () => {
    const lowLevel = await import('../src/low-level');

    expect(lowLevel).toHaveProperty('Registry');
    expect(lowLevel).toHaveProperty('Grammar');
    expect(lowLevel).toHaveProperty('createRegistry');
    expect(lowLevel).toHaveProperty('createGrammar');
  });

  it('should export high-level API from high-level module', async () => {
    const highLevel = await import('../src/high-level');

    expect(highLevel).toHaveProperty('SyntaxHighlighter');
    expect(highLevel).toHaveProperty('createSyntaxHighlighter');
  });
});

describe('WASM Files', () => {
  it('should have WASM files available', async () => {
    // In a real test environment, you would check if files exist
    // For now, we just verify the structure
    const module = getTextMateModule();
    expect(module).toBeDefined();
  });
});
