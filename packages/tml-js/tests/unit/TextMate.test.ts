import { describe, it, expect, beforeAll } from 'vitest';
import { TextMate, Registry, Grammar } from '../../src/index';
import { getGrammar, getTheme } from '../setup';

describe('TextMate', () => {
  let textMate: TextMate;

  beforeAll(async () => {
    textMate = await TextMate.create();
  });

  describe('create()', () => {
    it('should create a TextMate instance', async () => {
      const instance = await TextMate.create();
      expect(instance).toBeInstanceOf(TextMate);
    });
  });

  describe('getRegistry()', () => {
    it('should return a Registry instance', () => {
      const registry = textMate.getRegistry();
      expect(registry).toBeInstanceOf(Registry);
    });
  });

  describe('loadGrammar()', () => {
    it('should load a grammar from JSON content', async () => {
      const grammarJson = getGrammar('javascript');
      expect(grammarJson).not.toBeNull();
      const grammar = await textMate.loadGrammar('source.js', grammarJson!);
      expect(grammar).toBeInstanceOf(Grammar);
    });

    it('should return null for invalid grammar content', async () => {
      const grammar = await textMate.loadGrammar('source.invalid', 'not valid json');
      expect(grammar).toBeNull();
    });
  });

  describe('setTheme()', () => {
    it('should set a theme from JSON content', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      const result = textMate.setTheme(themeJson!);
      expect(result).toBe(true);
    });

    it('should return false for invalid theme content', () => {
      const result = textMate.setTheme('not valid json');
      expect(result).toBe(false);
    });
  });

  describe('getColorMap()', () => {
    it('should return an array of colors after setting a theme', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      textMate.setTheme(themeJson!);

      const colorMap = textMate.getColorMap();
      expect(Array.isArray(colorMap)).toBe(true);
      expect(colorMap.length).toBeGreaterThan(0);
    });

    it('should return colors as valid color strings', () => {
      const themeJson = getTheme('github-dark');
      expect(themeJson).not.toBeNull();
      textMate.setTheme(themeJson!);

      const colorMap = textMate.getColorMap();
      for (const color of colorMap) {
        expect(typeof color).toBe('string');
      }
    });
  });
});
