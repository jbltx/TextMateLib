import { grammars as grammarInfos } from 'tm-grammars';
import { themes as themeInfos } from 'tm-themes';
import { readFileSync } from 'fs';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));

/**
 * Get a grammar by its name
 * @param name The grammar name (e.g., 'javascript', 'python', 'html')
 * @returns The grammar JSON content as a string, or null if not found
 */
export function getGrammar(name: string): string | null {
  try {
    const grammarPath = resolve(__dirname, '../node_modules/tm-grammars/grammars', `${name}.json`);
    return readFileSync(grammarPath, 'utf-8');
  } catch {
    return null;
  }
}

/**
 * Get a theme by its name
 * @param name The theme name (e.g., 'github-dark', 'nord')
 * @returns The theme JSON content as a string, or null if not found
 */
export function getTheme(name: string): string | null {
  try {
    const themePath = resolve(__dirname, '../node_modules/tm-themes/themes', `${name}.json`);
    return readFileSync(themePath, 'utf-8');
  } catch {
    return null;
  }
}

/**
 * Get the list of available grammar names
 */
export function getAvailableGrammars(): string[] {
  return grammarInfos.map((g) => g.name);
}

/**
 * Get the list of available theme names
 */
export function getAvailableThemes(): string[] {
  return themeInfos.map((t) => t.name);
}

// Re-export metadata for convenience
export { grammarInfos, themeInfos };
