import { describe, it, expect, beforeAll } from 'vitest';
import { TextMate, Grammar } from '../../src/index';
import { getGrammar } from '../setup';

describe('Tokenization Integration', () => {
  let textMate: TextMate;
  let jsGrammar: Grammar;

  beforeAll(async () => {
    textMate = await TextMate.create();

    const jsGrammarJson = getGrammar('javascript');
    if (!jsGrammarJson) {
      throw new Error('Failed to get JavaScript grammar');
    }
    const loadedJsGrammar = await textMate.loadGrammar('source.js', jsGrammarJson);
    if (!loadedJsGrammar) {
      throw new Error('Failed to load JavaScript grammar');
    }
    jsGrammar = loadedJsGrammar;
  });

  describe('JavaScript tokenization', () => {
    it('should tokenize keywords', () => {
      const result = jsGrammar.tokenizeLine('function test() {}');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('storage.type') || s.includes('keyword'))).toBe(true);
    });

    it('should tokenize strings', () => {
      const result = jsGrammar.tokenizeLine('"hello world"');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('string'))).toBe(true);
    });

    it('should tokenize single-quoted strings', () => {
      const result = jsGrammar.tokenizeLine("'hello world'");
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('string'))).toBe(true);
    });

    it('should tokenize comments', () => {
      const result = jsGrammar.tokenizeLine('// this is a comment');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('comment'))).toBe(true);
    });

    it('should tokenize block comments', () => {
      const result = jsGrammar.tokenizeLine('/* block comment */');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('comment'))).toBe(true);
    });

    it('should tokenize numbers', () => {
      const result = jsGrammar.tokenizeLine('const x = 42;');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('constant.numeric'))).toBe(true);
    });

    it('should tokenize hexadecimal numbers', () => {
      const result = jsGrammar.tokenizeLine('const x = 0xFF;');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('constant.numeric'))).toBe(true);
    });

    it('should tokenize boolean literals', () => {
      const result = jsGrammar.tokenizeLine('const flag = true;');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('constant.language'))).toBe(true);
    });

    it('should tokenize control flow keywords', () => {
      const result = jsGrammar.tokenizeLine('if (x) { return y; }');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('keyword.control'))).toBe(true);
    });
  });

  describe('Token position validation', () => {
    it('should produce non-overlapping tokens', () => {
      const result = jsGrammar.tokenizeLine('const x = 42;');
      const tokens = result.tokens;

      for (let i = 0; i < tokens.length - 1; i++) {
        expect(tokens[i].endIndex).toBeLessThanOrEqual(tokens[i + 1].startIndex);
      }
    });

    it('should cover the entire line', () => {
      const line = 'const x = 42;';
      const result = jsGrammar.tokenizeLine(line);
      const tokens = result.tokens;

      if (tokens.length > 0) {
        expect(tokens[0].startIndex).toBe(0);
        expect(tokens[tokens.length - 1].endIndex).toBe(line.length);
      }
    });

    it('should have valid token ranges', () => {
      const result = jsGrammar.tokenizeLine('function test() {}');

      for (const token of result.tokens) {
        expect(token.startIndex).toBeGreaterThanOrEqual(0);
        expect(token.endIndex).toBeGreaterThan(token.startIndex);
      }
    });
  });

  describe('Multi-line tokenization', () => {
    it('should handle multi-line block comments', () => {
      const line1 = '/* start of comment';
      const line2 = 'end of comment */';

      const result1 = jsGrammar.tokenizeLine(line1);
      const result2 = jsGrammar.tokenizeLine(line2, result1.ruleStack);

      const scopes2 = result2.tokens.flatMap((t) => t.scopes);
      expect(scopes2.some((s) => s.includes('comment'))).toBe(true);
    });

    it('should maintain state across lines', () => {
      const lines = ['function test() {', '  return 42;', '}'];

      let ruleStack = null;
      const allTokens = [];

      for (const line of lines) {
        const result = jsGrammar.tokenizeLine(line, ruleStack);
        ruleStack = result.ruleStack;
        allTokens.push(...result.tokens);
      }

      expect(allTokens.length).toBeGreaterThan(0);
    });
  });

  describe('Python tokenization', () => {
    let pyGrammar: Grammar;

    beforeAll(async () => {
      const pyGrammarJson = getGrammar('python');
      if (!pyGrammarJson) {
        throw new Error('Failed to get Python grammar');
      }
      const loadedGrammar = await textMate.loadGrammar('source.python', pyGrammarJson);
      if (!loadedGrammar) {
        throw new Error('Failed to load Python grammar');
      }
      pyGrammar = loadedGrammar;
    });

    it('should tokenize Python keywords', () => {
      const result = pyGrammar.tokenizeLine('def hello():');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('keyword') || s.includes('storage'))).toBe(true);
    });

    it('should tokenize Python strings', () => {
      const result = pyGrammar.tokenizeLine('x = "hello world"');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('string'))).toBe(true);
    });

    it('should tokenize Python comments', () => {
      const result = pyGrammar.tokenizeLine('# this is a comment');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('comment'))).toBe(true);
    });
  });

  describe('TypeScript tokenization', () => {
    let tsGrammar: Grammar;

    beforeAll(async () => {
      const tsGrammarJson = getGrammar('typescript');
      if (!tsGrammarJson) {
        throw new Error('Failed to get TypeScript grammar');
      }
      const loadedGrammar = await textMate.loadGrammar('source.ts', tsGrammarJson);
      if (!loadedGrammar) {
        throw new Error('Failed to load TypeScript grammar');
      }
      tsGrammar = loadedGrammar;
    });

    it('should tokenize TypeScript type annotations', () => {
      const result = tsGrammar.tokenizeLine('const x: number = 42;');
      expect(result.tokens.length).toBeGreaterThan(0);
    });

    it('should tokenize TypeScript interfaces', () => {
      const result = tsGrammar.tokenizeLine('interface User { name: string }');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('keyword') || s.includes('storage'))).toBe(true);
    });
  });
});
