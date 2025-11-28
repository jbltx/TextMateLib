// Grammar and theme list modules
import { getAvailableGrammars } from './grammars.js';
import { getAvailableThemes } from './themes.js';

// Application state
let textMateModule = null;
let currentGrammar = null;
let currentGrammarHandle = null;
let currentTheme = null;
let registry = null;
let grammars = [];
let themes = [];

// Sample code templates for different languages
const sampleCode = {
    javascript: `// JavaScript Example
function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

const result = fibonacci(10);
console.log("Fibonacci(10):", result);
`,
    python: `# Python Example
def quick_sort(arr):
    if len(arr) <= 1:
        return arr
    pivot = arr[len(arr) // 2]
    left = [x for x in arr if x < pivot]
    middle = [x for x in arr if x == pivot]
    right = [x for x in arr if x > pivot]
    return quick_sort(left) + middle + quick_sort(right)

numbers = [3, 6, 8, 10, 1, 2, 1]
print(quick_sort(numbers))
`,
    cpp: `// C++ Example
#include <iostream>
#include <vector>

template<typename T>
class Stack {
private:
    std::vector<T> elements;
public:
    void push(const T& elem) {
        elements.push_back(elem);
    }
    
    T pop() {
        if (elements.empty()) {
            throw std::out_of_range("Stack is empty");
        }
        T elem = elements.back();
        elements.pop_back();
        return elem;
    }
};

int main() {
    Stack<int> stack;
    stack.push(42);
    return 0;
}
`,
    typescript: `// TypeScript Example
interface Person {
    name: string;
    age: number;
}

class Employee implements Person {
    constructor(
        public name: string,
        public age: number,
        private salary: number
    ) {}
    
    getDetails(): string {
        return \`\${this.name} (age \${this.age})\`;
    }
}

const emp = new Employee("Alice", 30, 50000);
console.log(emp.getDetails());
`,
    rust: `// Rust Example
fn main() {
    let numbers = vec![1, 2, 3, 4, 5];
    
    let sum: i32 = numbers.iter().sum();
    let doubled: Vec<i32> = numbers.iter()
        .map(|x| x * 2)
        .collect();
    
    println!("Sum: {}", sum);
    println!("Doubled: {:?}", doubled);
}
`,
    html: `<!-- HTML Example -->
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Hello World</title>
    <style>
        body { font-family: Arial, sans-serif; }
        .container { max-width: 800px; margin: 0 auto; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Hello, World!</h1>
        <p>This is a sample HTML document.</p>
    </div>
</body>
</html>
`
};

// Initialize the application
async function init() {
    try {
        // Load grammar and theme lists
        grammars = await getAvailableGrammars();
        themes = await getAvailableThemes();
        
        // Populate dropdowns
        populateGrammarSelect();
        populateThemeSelect();
        
        // Set up event listeners
        document.getElementById('grammarSelect').addEventListener('change', onGrammarChange);
        document.getElementById('themeSelect').addEventListener('change', onThemeChange);
        document.getElementById('toggleDebug').addEventListener('click', toggleDebugView);
        document.getElementById('codeEditor').addEventListener('input', onCodeChange);
        
        // Try to load WASM module if available
        await loadWasmModule();
        
        // Set initial code
        setInitialCode();
        
    } catch (error) {
        console.error('Initialization error:', error);
        showError('Failed to initialize playground: ' + error.message);
    }
}

// Load WASM module
async function loadWasmModule() {
    const loadingStatus = document.getElementById('loadingStatus');
    
    try {
        // Check if WASM files exist
        const response = await fetch('wasm/tml-standard.js');
        if (!response.ok) {
            throw new Error('WASM module not found. Please build it first using: ./scripts/build-wasm-standard.sh');
        }
        
        // Load the WASM module
        const script = document.createElement('script');
        script.src = 'wasm/tml-standard.js';
        document.head.appendChild(script);
        
        // Wait for module to load
        await new Promise((resolve, reject) => {
            script.onload = resolve;
            script.onerror = () => reject(new Error('Failed to load WASM module'));
        });
        
        // Initialize the module
        if (typeof createTextMateModule === 'function') {
            textMateModule = await createTextMateModule();
            registry = new textMateModule.Registry();
            loadingStatus.classList.add('hidden');
            console.log('TextMateLib WASM module loaded successfully');
        } else {
            throw new Error('WASM module did not export expected function');
        }
        
    } catch (error) {
        console.warn('WASM module not available:', error.message);
        loadingStatus.innerHTML = `
            <p style="color: #dc3545;">WASM Module Not Available</p>
            <p style="font-size: 0.9em; margin-top: 10px;">
                The playground is running in demo mode with mock highlighting.<br>
                To enable real syntax highlighting, build the WASM module:<br>
                <code style="background: #f8f9fa; padding: 5px 10px; border-radius: 4px; display: inline-block; margin-top: 10px;">
                    ./scripts/build-wasm-standard.sh
                </code>
            </p>
        `;
        setTimeout(() => {
            loadingStatus.classList.add('hidden');
        }, 5000);
    }
}

// Populate grammar select dropdown
function populateGrammarSelect() {
    const select = document.getElementById('grammarSelect');
    select.innerHTML = '<option value="">Select a language...</option>';
    
    grammars.forEach(grammar => {
        const option = document.createElement('option');
        option.value = grammar.name;
        option.textContent = grammar.displayName || grammar.name;
        select.appendChild(option);
    });
    
    // Select JavaScript by default if available
    if (grammars.some(g => g.name === 'javascript')) {
        select.value = 'javascript';
        onGrammarChange();
    }
}

// Populate theme select dropdown
function populateThemeSelect() {
    const select = document.getElementById('themeSelect');
    select.innerHTML = '<option value="">Select a theme...</option>';
    
    themes.forEach(theme => {
        const option = document.createElement('option');
        option.value = theme.name;
        option.textContent = theme.displayName || theme.name;
        select.appendChild(option);
    });
    
    // Select dark-plus by default if available
    if (themes.some(t => t.name === 'dark-plus')) {
        select.value = 'dark-plus';
        onThemeChange();
    }
}

// Handle grammar change
async function onGrammarChange() {
    const select = document.getElementById('grammarSelect');
    const grammarName = select.value;
    
    if (!grammarName) return;
    
    const grammar = grammars.find(g => g.name === grammarName);
    if (!grammar) return;
    
    currentGrammar = grammar;
    
    // Load grammar if WASM is available
    if (registry && grammar.path) {
        try {
            const response = await fetch(grammar.path);
            const grammarJson = await response.text();
            currentGrammarHandle = registry.loadGrammarFromContent(grammarJson, grammar.scopeName);
            
            if (currentGrammarHandle) {
                console.log('Grammar loaded:', grammar.name);
            }
        } catch (error) {
            console.error('Failed to load grammar:', error);
        }
    }
    
    // Update sample code
    updateSampleCode();
    
    // Re-highlight
    highlightCode();
}

// Handle theme change
async function onThemeChange() {
    const select = document.getElementById('themeSelect');
    const themeName = select.value;
    
    if (!themeName) return;
    
    const theme = themes.find(t => t.name === themeName);
    if (!theme) return;
    
    currentTheme = theme;
    
    // Load theme if available
    if (theme.path) {
        try {
            const response = await fetch(theme.path);
            const themeData = await response.json();
            applyTheme(themeData);
        } catch (error) {
            console.error('Failed to load theme:', error);
        }
    }
    
    // Re-highlight
    highlightCode();
}

// Apply theme colors
function applyTheme(themeData) {
    const output = document.getElementById('highlightedOutput');
    
    if (themeData.colors && themeData.colors['editor.background']) {
        output.style.background = themeData.colors['editor.background'];
    }
    
    if (themeData.colors && themeData.colors['editor.foreground']) {
        output.style.color = themeData.colors['editor.foreground'];
    }
}

// Update sample code based on selected grammar
function updateSampleCode() {
    const editor = document.getElementById('codeEditor');
    const grammarName = currentGrammar?.name;
    
    if (editor.value.trim() === '' && grammarName && sampleCode[grammarName]) {
        editor.value = sampleCode[grammarName];
    }
}

// Set initial code
function setInitialCode() {
    const editor = document.getElementById('codeEditor');
    editor.value = sampleCode.javascript;
}

// Handle code change
function onCodeChange() {
    highlightCode();
}

// Highlight code
function highlightCode() {
    const code = document.getElementById('codeEditor').value;
    const output = document.getElementById('highlightedOutput');
    const debugOutput = document.getElementById('debugOutput');
    
    if (!currentGrammar) {
        output.innerHTML = '<pre>' + escapeHtml(code) + '</pre>';
        debugOutput.innerHTML = '<p>Please select a grammar first.</p>';
        return;
    }
    
    if (textMateModule && registry) {
        // Real highlighting with WASM
        highlightWithWasm(code, output, debugOutput);
    } else {
        // Mock highlighting for demo
        mockHighlight(code, output, debugOutput);
    }
}

// Highlight with WASM module
function highlightWithWasm(code, output, debugOutput) {
    try {
        const lines = code.split('\n');
        let htmlOutput = '<pre>';
        let debugInfo = '';
        let ruleStack = null;
        
        lines.forEach((line, lineNum) => {
            const grammarWrapper = new textMateModule.Grammar(currentGrammarHandle);
            const result = grammarWrapper.tokenizeLine(line, ruleStack);
            
            ruleStack = result.ruleStack;
            
            // Build HTML output
            let lineHtml = '';
            result.tokens.forEach(token => {
                const text = line.substring(token.startIndex, token.endIndex);
                const scopes = token.scopes.join(' ');
                lineHtml += `<span class="${scopes}">${escapeHtml(text)}</span>`;
            });
            
            htmlOutput += lineHtml + '\n';
            
            // Build debug info
            debugInfo += `<div class="token-debug">
                <div class="token-debug-line">Line ${lineNum + 1}</div>`;
            
            result.tokens.forEach(token => {
                const text = line.substring(token.startIndex, token.endIndex);
                debugInfo += `<div class="token-info">
                    <span class="token-text">${escapeHtml(text)}</span>
                    <span class="token-scopes">${token.scopes.join(' › ')}</span>
                    <div class="token-color" style="background: #667eea;"></div>
                </div>`;
            });
            
            debugInfo += '</div>';
        });
        
        htmlOutput += '</pre>';
        output.innerHTML = htmlOutput;
        debugOutput.innerHTML = debugInfo;
        
    } catch (error) {
        console.error('Highlighting error:', error);
        output.innerHTML = '<pre>' + escapeHtml(code) + '</pre>';
        debugOutput.innerHTML = '<p style="color: red;">Error: ' + escapeHtml(error.message) + '</p>';
    }
}

// Mock highlighting for demo mode
function mockHighlight(code, output, debugOutput) {
    const lines = code.split('\n');
    let htmlOutput = '<pre>';
    let debugInfo = '';
    
    // Simple regex-based highlighting for demo
    const keywords = /\b(function|const|let|var|if|else|return|class|import|export|from|async|await|for|while|def|print|include|namespace|public|private|protected)\b/g;
    const strings = /(["'`])(?:(?=(\\?))\2.)*?\1/g;
    const comments = /(\/\/.*$|\/\*[\s\S]*?\*\/|#.*$)/gm;
    const numbers = /\b\d+\.?\d*\b/g;
    
    lines.forEach((line, lineNum) => {
        let highlighted = escapeHtml(line);
        
        // Apply simple highlighting
        highlighted = highlighted.replace(comments, '<span style="color: #6a9955;">$&</span>');
        highlighted = highlighted.replace(strings, '<span style="color: #ce9178;">$&</span>');
        highlighted = highlighted.replace(keywords, '<span style="color: #569cd6;">$&</span>');
        highlighted = highlighted.replace(numbers, '<span style="color: #b5cea8;">$&</span>');
        
        htmlOutput += highlighted + '\n';
        
        // Build simple debug info
        debugInfo += `<div class="token-debug">
            <div class="token-debug-line">Line ${lineNum + 1} (Mock Mode)</div>
            <div class="token-info">
                <span class="token-text">${escapeHtml(line.substring(0, 50))}${line.length > 50 ? '...' : ''}</span>
                <span class="token-scopes">source.${currentGrammar.name}</span>
                <div class="token-color" style="background: #d4d4d4;"></div>
            </div>
        </div>`;
    });
    
    htmlOutput += '</pre>';
    output.innerHTML = htmlOutput;
    debugOutput.innerHTML = debugInfo;
}

// Toggle debug view
function toggleDebugView() {
    const debugPanel = document.getElementById('debugPanel');
    if (debugPanel.style.display === 'none') {
        debugPanel.style.display = 'block';
    } else {
        debugPanel.style.display = 'none';
    }
}

// Utility function to escape HTML
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Show error message
function showError(message) {
    const loadingStatus = document.getElementById('loadingStatus');
    loadingStatus.innerHTML = `<p style="color: #dc3545;">${escapeHtml(message)}</p>`;
    loadingStatus.classList.remove('hidden');
}

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
