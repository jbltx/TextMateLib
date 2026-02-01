import { describe, it, expect, beforeAll, beforeEach } from 'vitest';
import { TextMate, Grammar } from '../../src/index';
import { getGrammar, getTheme } from '../setup';

describe('Theme Integration', () => {
  let textMate: TextMate;
  let grammar: Grammar;

  beforeAll(async () => {
    textMate = await TextMate.create();

    const grammarJson = getGrammar('javascript');
    if (!grammarJson) {
      throw new Error('Failed to get JavaScript grammar');
    }
    const loadedGrammar = await textMate.loadGrammar('source.js', grammarJson);
    if (!loadedGrammar) {
      throw new Error('Failed to load JavaScript grammar');
    }
    grammar = loadedGrammar;
  });

  describe('Color map', () => {
    it('should populate color map after setting theme', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      const result = textMate.setTheme(themeJson!);
      expect(result).toBe(true);

      const colorMap = textMate.getColorMap();
      expect(colorMap.length).toBeGreaterThan(0);
    });

    it('should return string colors', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      textMate.setTheme(themeJson!);

      const colorMap = textMate.getColorMap();
      for (const color of colorMap) {
        expect(typeof color).toBe('string');
      }
    });

    it('should return consistent color map on multiple calls', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      textMate.setTheme(themeJson!);

      const colorMap1 = textMate.getColorMap();
      const colorMap2 = textMate.getColorMap();

      expect(colorMap1).toEqual(colorMap2);
    });
  });

  describe('Tokenization with theme', () => {
    beforeEach(() => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      textMate.setTheme(themeJson!);
    });

    it('should tokenize code with active theme', () => {
      const result = grammar.tokenizeLine('const x = 42;');
      expect(result.tokens.length).toBeGreaterThan(0);
    });

    it('should produce binary tokens with theme metadata', () => {
      const result = grammar.tokenizeLine2('const x = 42;');
      expect(result.tokens.length).toBeGreaterThan(0);

      for (const token of result.tokens) {
        expect(typeof token).toBe('number');
      }
    });

    it('should tokenize different token types', () => {
      const result = grammar.tokenizeLine('function test() { return "hello"; }');
      expect(result.tokens.length).toBeGreaterThan(0);

      const scopes = result.tokens.flatMap((t) => t.scopes);
      expect(scopes.some((s) => s.includes('string'))).toBe(true);
    });
  });

  describe('Theme switching', () => {
    it('should allow setting different themes', () => {
      const githubDark = getTheme('github-dark');
      expect(githubDark).not.toBeNull();
      const result1 = textMate.setTheme(githubDark!);
      expect(result1).toBe(true);

      const colorMap1 = textMate.getColorMap();
      expect(colorMap1.length).toBeGreaterThan(0);

      const nord = getTheme('nord');
      expect(nord).not.toBeNull();
      const result2 = textMate.setTheme(nord!);
      expect(result2).toBe(true);

      const colorMap2 = textMate.getColorMap();
      expect(colorMap2.length).toBeGreaterThan(0);
    });

    it('should maintain tokenization after theme change', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      textMate.setTheme(themeJson!);

      const result1 = grammar.tokenizeLine('const x = 42;');
      expect(result1.tokens.length).toBeGreaterThan(0);

      const nordTheme = getTheme('nord');
      expect(nordTheme).not.toBeNull();
      textMate.setTheme(nordTheme!);

      const result2 = grammar.tokenizeLine('const x = 42;');
      expect(result2.tokens.length).toBeGreaterThan(0);
    });
  });

  describe('Theme validation', () => {
    it('should reject invalid theme JSON', () => {
      const result = textMate.setTheme('not valid json');
      expect(result).toBe(false);
    });

    it('should reject empty theme content', () => {
      const result = textMate.setTheme('');
      expect(result).toBe(false);
    });

    it('should accept valid theme JSON', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      const result = textMate.setTheme(themeJson!);
      expect(result).toBe(true);
    });
  });

  describe('Multiple themes', () => {
    it('should work with github-light theme', () => {
      const theme = getTheme('github-light');
      expect(theme).not.toBeNull();
      const result = textMate.setTheme(theme!);
      expect(result).toBe(true);

      const colorMap = textMate.getColorMap();
      expect(colorMap.length).toBeGreaterThan(0);
    });

    it('should work with dracula theme', () => {
      const theme = getTheme('dracula');
      expect(theme).not.toBeNull();
      const result = textMate.setTheme(theme!);
      expect(result).toBe(true);

      const colorMap = textMate.getColorMap();
      expect(colorMap.length).toBeGreaterThan(0);
    });

    it('should work with one-dark-pro theme', () => {
      const theme = getTheme('one-dark-pro');
      expect(theme).not.toBeNull();
      const result = textMate.setTheme(theme!);
      expect(result).toBe(true);

      const colorMap = textMate.getColorMap();
      expect(colorMap.length).toBeGreaterThan(0);
    });
  });
});
