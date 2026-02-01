import { describe, it, expect, beforeAll } from 'vitest';
import { TextMate, Grammar } from '../../src/index';
import { getGrammar } from '../setup';

describe('Grammar', () => {
  let grammar: Grammar;

  beforeAll(async () => {
    const textMate = await TextMate.create();
    const grammarJson = getGrammar('javascript');
    if (!grammarJson) {
      throw new Error('Failed to get JavaScript grammar');
    }
    const loadedGrammar = textMate.getRegistry().loadGrammar('source.js', grammarJson);
    if (!loadedGrammar) {
      throw new Error('Failed to load grammar');
    }
    grammar = loadedGrammar;
  });

  describe('getScopeName()', () => {
    it('should return the scope name of the grammar', () => {
      const scopeName = grammar.getScopeName();
      expect(scopeName).toBe('source.js');
    });
  });

  describe('tokenizeLine()', () => {
    it('should tokenize a simple line', () => {
      const result = grammar.tokenizeLine('const x = 42;');
      expect(result).toBeDefined();
      expect(result.tokens).toBeDefined();
      expect(Array.isArray(result.tokens)).toBe(true);
    });

    it('should return tokens with correct structure', () => {
      const result = grammar.tokenizeLine('const x = 42;');
      expect(result.tokens.length).toBeGreaterThan(0);

      for (const token of result.tokens) {
        expect(typeof token.startIndex).toBe('number');
        expect(typeof token.endIndex).toBe('number');
        expect(Array.isArray(token.scopes)).toBe(true);
      }
    });

    it('should return a rule stack for continuation', () => {
      const result = grammar.tokenizeLine('function test() {');
      expect(result.ruleStack).toBeDefined();
    });

    it('should handle multi-line tokenization with ruleStack', () => {
      const line1Result = grammar.tokenizeLine('function test() {');
      const line2Result = grammar.tokenizeLine('  return 42;', line1Result.ruleStack);

      expect(line2Result.tokens).toBeDefined();
      expect(line2Result.ruleStack).toBeDefined();
    });

    it('should handle empty lines', () => {
      const result = grammar.tokenizeLine('');
      expect(result).toBeDefined();
      expect(result.tokens).toBeDefined();
    });
  });

  describe('tokenizeLine2()', () => {
    it('should tokenize a line with binary format', () => {
      const result = grammar.tokenizeLine2('const x = 42;');
      expect(result).toBeDefined();
      expect(result.tokens).toBeDefined();
      expect(Array.isArray(result.tokens)).toBe(true);
    });

    it('should return tokens as numbers', () => {
      const result = grammar.tokenizeLine2('const x = 42;');
      expect(result.tokens.length).toBeGreaterThan(0);

      for (const token of result.tokens) {
        expect(typeof token).toBe('number');
      }
    });

    it('should return a rule stack for continuation', () => {
      const result = grammar.tokenizeLine2('function test() {');
      expect(result.ruleStack).toBeDefined();
    });

    it('should handle multi-line tokenization with ruleStack', () => {
      const line1Result = grammar.tokenizeLine2('function test() {');
      const line2Result = grammar.tokenizeLine2('  return 42;', line1Result.ruleStack);

      expect(line2Result.tokens).toBeDefined();
      expect(line2Result.ruleStack).toBeDefined();
    });
  });
});
