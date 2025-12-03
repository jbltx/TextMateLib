/**
 * Low-level API example
 * Direct use of Registry and Grammar for advanced tokenization control
 */

import { createRegistry } from '../dist/low-level.mjs';
import fs from 'fs';

async function main() {
  // Load grammar and theme files
  // You can download these from:
  // - VS Code: https://github.com/microsoft/vscode/tree/main/extensions
  // - TextMate: https://github.com/textmate/textmate
  const grammarContent = fs.readFileSync('./javascript.json', 'utf-8');
  const themeContent = fs.readFileSync('./dracula.json', 'utf-8');

  // Create registry
  console.log('Creating registry...');
  const registry = await createRegistry();

  // Load grammar
  console.log('Loading JavaScript grammar...');
  const grammar = registry.loadGrammarFromContent(grammarContent, 'source.javascript');

  // Set theme
  console.log('Loading Dracula theme...');
  registry.setTheme(themeContent);

  // Example code to highlight
  const code = `function fibonacci(n) {
  if (n <= 1) return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}`;

  console.log('\n--- Tokenizing with state management ---\n');

  const lines = code.split('\n');
  let ruleStack = null; // Start with no state

  for (let lineNum = 0; lineNum < lines.length; lineNum++) {
    const line = lines[lineNum];
    console.log(`Line ${lineNum + 1}: "${line}"`);

    // Tokenize this line, passing the rule stack from the previous line
    const result = grammar.tokenizeLine(line, ruleStack);

    // Display tokens
    result.tokens.forEach((token, idx) => {
      const text = line.substring(token.startIndex, token.endIndex);
      console.log(`  Token ${idx}: "${text}"`);
      console.log(`    Scopes: ${token.scopes.join(' > ')}`);
    });

    // Save rule stack for next line
    ruleStack = result.ruleStack;
    console.log(`  Rule stack: ${ruleStack ? 'has state' : 'null'}`);
    console.log();
  }

  // Get theme color map
  console.log('--- Theme Colors ---');
  const colorMap = registry.getColorMap();
  console.log(`Theme has ${colorMap.length} colors`);
  colorMap.slice(0, 5).forEach((color, idx) => {
    console.log(`  Color ${idx}: ${color}`);
  });
}

main().catch(console.error);
