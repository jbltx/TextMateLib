import { describe, it, expect, beforeAll } from 'vitest';
import { TextMate, Registry, Grammar } from '../../src/index';
import { getGrammar, getTheme } from '../setup';

describe('Registry', () => {
  let registry: Registry;

  beforeAll(async () => {
    const textMate = await TextMate.create();
    registry = textMate.getRegistry();
  });

  describe('loadGrammar()', () => {
    it('should load a JavaScript grammar', () => {
      const grammarJson = getGrammar('javascript');
      expect(grammarJson).not.toBeNull();
      const grammar = registry.loadGrammar('source.js', grammarJson!);
      expect(grammar).toBeInstanceOf(Grammar);
    });

    it('should load a Python grammar', () => {
      const grammarJson = getGrammar('python');
      expect(grammarJson).not.toBeNull();
      const grammar = registry.loadGrammar('source.python', grammarJson!);
      expect(grammar).toBeInstanceOf(Grammar);
    });

    it('should load a TypeScript grammar', () => {
      const grammarJson = getGrammar('typescript');
      expect(grammarJson).not.toBeNull();
      const grammar = registry.loadGrammar('source.ts', grammarJson!);
      expect(grammar).toBeInstanceOf(Grammar);
    });

    it('should return null for invalid grammar content', () => {
      const grammar = registry.loadGrammar('source.invalid', 'invalid json');
      expect(grammar).toBeNull();
    });

    it('should return null for empty grammar content', () => {
      const grammar = registry.loadGrammar('source.empty', '');
      expect(grammar).toBeNull();
    });
  });

  describe('setTheme()', () => {
    it('should set a theme from JSON content', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      const result = registry.setTheme(themeJson!);
      expect(result).toBe(true);
    });

    it('should set different themes', () => {
      const nordTheme = getTheme('nord');
      expect(nordTheme).not.toBeNull();
      const result = registry.setTheme(nordTheme!);
      expect(result).toBe(true);
    });

    it('should return false for invalid theme content', () => {
      const result = registry.setTheme('invalid json');
      expect(result).toBe(false);
    });
  });

  describe('getColorMap()', () => {
    it('should return an array of colors', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      registry.setTheme(themeJson!);

      const colorMap = registry.getColorMap();
      expect(Array.isArray(colorMap)).toBe(true);
    });

    it('should contain colors after setting a theme', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      registry.setTheme(themeJson!);

      const colorMap = registry.getColorMap();
      expect(colorMap.length).toBeGreaterThan(0);
    });
  });
});
