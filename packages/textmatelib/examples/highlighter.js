/**
 * High-level API example
 * Simple one-liner highlighting with colors and styles
 */

import { createSyntaxHighlighter } from '../dist/high-level.mjs';
import fs from 'fs';

async function main() {
  // Load grammar and theme files
  // You can download these from:
  // - VS Code: https://github.com/microsoft/vscode/tree/main/extensions
  // - TextMate: https://github.com/textmate/textmate
  const grammarContent = fs.readFileSync('./javascript.json', 'utf-8');
  const themeContent = fs.readFileSync('./dracula.json', 'utf-8');

  // Create highlighter with grammar and theme
  console.log('Creating syntax highlighter...');
  const highlighter = await createSyntaxHighlighter({
    grammarContent,
    themeContent,
    scopeName: 'source.javascript',
  });

  // Example code to highlight
  const code = `// Quick sort implementation
function quickSort(arr) {
  if (arr.length <= 1) return arr;
  const pivot = arr[Math.floor(arr.length / 2)];
  return [
    ...quickSort(arr.filter(x => x < pivot)),
    ...arr.filter(x => x === pivot),
    ...quickSort(arr.filter(x => x > pivot))
  ];
}`;

  console.log('--- Highlighting Code ---\n');

  // Highlight all lines at once
  const highlighted = highlighter.highlight(code);

  const lines = code.split('\n');
  for (let lineNum = 0; lineNum < lines.length; lineNum++) {
    const line = lines[lineNum];
    const tokens = highlighted[lineNum];

    console.log(`Line ${lineNum + 1}:`);
    console.log(`  Source: "${line}"`);

    tokens.forEach((token, idx) => {
      const text = line.substring(token.startIndex, token.endIndex);
      console.log(`  Token ${idx}: "${text}"`);
      console.log(`    Scopes: ${token.scopes.join(' > ')}`);

      if (token.foreground) {
        console.log(`    Foreground: ${token.foreground}`);
      }
      if (token.fontStyle) {
        console.log(`    Style: ${token.fontStyle}`);
      }
    });

    console.log();
  }

  // Example: Extract all keywords with their colors
  console.log('--- All Keywords (with colors) ---\n');
  let lineNum = 0;
  for (const tokens of highlighted) {
    const line = lines[lineNum];
    for (const token of tokens) {
      const text = line.substring(token.startIndex, token.endIndex);
      // Check if this is a keyword scope
      if (token.scopes.some((scope) => scope.includes('keyword'))) {
        console.log(`"${text}" -> ${token.foreground || 'default'}`);
      }
    }
    lineNum++;
  }

  // Example: Update grammar and theme dynamically
  console.log('\n--- Changing Grammar and Theme ---\n');

  // Load Python grammar
  const pythonGrammar = fs.readFileSync('./python.json', 'utf-8');
  const pytonTheme = fs.readFileSync('./monokai.json', 'utf-8');

  highlighter.setGrammar(pythonGrammar, 'source.python');
  highlighter.setTheme(pytonTheme);

  const pythonCode = `def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)`;

  const pythonHighlighted = highlighter.highlight(pythonCode);
  console.log('Python code highlighted with Monokai theme:');
  pythonHighlighted.forEach((tokens, lineNum) => {
    console.log(`  Line ${lineNum + 1}: ${tokens.length} tokens`);
  });
}

main().catch(console.error);
