import { describe, it, expect, beforeAll } from 'vitest';
import { TextMate, Grammar } from '../../src/index';
import { getGrammar } from '../setup';

describe('UTF-16 token index alignment', () => {
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

    // Warmup: first tokenization compiles all Oniguruma regex patterns
    grammar.tokenizeLine('const x = 0;');
  });

  /**
   * Helper: reconstruct the full line from token ranges using JS string.substring().
   * If indices are UTF-8 byte offsets instead of UTF-16 code unit indices,
   * the reconstruction will fail for any non-ASCII input.
   */
  function reconstructLine(line: string, tokens: { startIndex: number; endIndex: number }[]): string {
    return tokens.map((t) => line.substring(t.startIndex, t.endIndex)).join('');
  }

  describe('tokenizeLine() — accented characters (2-byte UTF-8, 1 UTF-16 code unit)', () => {
    it('should align token indices for "café"', () => {
      const line = 'const x = "café";';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });

    it('should align token indices for accented identifiers', () => {
      const line = 'const naïveté = 42;';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });
  });

  describe('tokenizeLine() — 3-byte UTF-8 characters (1 UTF-16 code unit)', () => {
    it('should align token indices for ellipsis (U+2026)', () => {
      const line = 'const x = "hello\u2026world";';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });

    it('should align token indices for CJK characters', () => {
      const line = 'const x = "\u540D\u524D\u30C6\u30B9\u30C8";';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });
  });

  describe('tokenizeLine() — 4-byte UTF-8 / surrogate pairs (2 UTF-16 code units)', () => {
    it('should align token indices for emoji (U+1F600)', () => {
      const line = 'const x = "\u{1F600}";';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });

    it('should align token indices for multiple emoji', () => {
      const line = 'const x = "\u{1F600}\u{1F680}\u{1F4A9}";';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });

    it('should align token indices when emoji appears before keywords', () => {
      const line = '\u{1F600} return 42';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });
  });

  describe('tokenizeLine() — mixed multi-byte sequences', () => {
    it('should align indices for mixed 2-byte, 3-byte, and 4-byte chars', () => {
      const line = 'const caf\u00E9 = "\u{1F600}\u2026";';
      const result = grammar.tokenizeLine(line);

      expect(result.tokens.length).toBeGreaterThan(0);
      expect(reconstructLine(line, result.tokens)).toBe(line);
    });

    it('should cover the entire line length', () => {
      const line = 'const x = "\u{1F600}caf\u00E9\u2026";';
      const result = grammar.tokenizeLine(line);
      const tokens = result.tokens;

      if (tokens.length > 0) {
        expect(tokens[0].startIndex).toBe(0);
        expect(tokens[tokens.length - 1].endIndex).toBe(line.length);
      }
    });

    it('should produce non-overlapping tokens', () => {
      const line = 'const caf\u00E9 = "\u{1F600}\u{1F680}";';
      const result = grammar.tokenizeLine(line);
      const tokens = result.tokens;

      for (let i = 0; i < tokens.length - 1; i++) {
        expect(tokens[i].endIndex).toBeLessThanOrEqual(tokens[i + 1].startIndex);
      }
    });
  });

  describe('tokenizeLine2() — binary format with non-ASCII', () => {
    it('should return valid encoded tokens for accented characters', () => {
      const line = 'const x = "café";';
      const result = grammar.tokenizeLine2(line);

      expect(result.tokens.length).toBeGreaterThan(0);

      // Encoded tokens are pairs: [startIndex, metadata, ...]
      // The last start offset should not exceed line.length
      for (let i = 0; i < result.tokens.length; i += 2) {
        expect(result.tokens[i]).toBeLessThanOrEqual(line.length);
      }
    });

    it('should return valid encoded tokens for emoji', () => {
      const line = 'const x = "\u{1F600}";';
      const result = grammar.tokenizeLine2(line);

      expect(result.tokens.length).toBeGreaterThan(0);

      for (let i = 0; i < result.tokens.length; i += 2) {
        expect(result.tokens[i]).toBeLessThanOrEqual(line.length);
      }
    });
  });

  describe('multi-line tokenization with non-ASCII', () => {
    it('should maintain correct state across lines with non-ASCII content', () => {
      const lines = [
        'const café = "hello";',
        'const x = "\u{1F600}";',
        'return café;',
      ];

      let ruleStack = null;
      for (const line of lines) {
        const result = grammar.tokenizeLine(line, ruleStack);
        ruleStack = result.ruleStack;

        expect(result.tokens.length).toBeGreaterThan(0);
        expect(reconstructLine(line, result.tokens)).toBe(line);
      }
    });
  });
});
