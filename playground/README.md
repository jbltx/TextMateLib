# TextMateLib Playground

A web-based playground for exploring TextMate syntax highlighting with various grammars and themes.

## Features

- 📝 **Code Editor**: Write or paste code in any supported language
- 🎨 **Theme Selection**: Choose from 40+ popular VS Code themes
- 🔤 **Language Support**: 30+ programming languages with TextMate grammars
- 🐛 **Debug View**: See detailed tokenization information for learning and debugging
- ⚡ **Real-time Preview**: See syntax highlighting updates as you type
- 🌈 **Beautiful UI**: Modern, responsive design with smooth animations

## Quick Start

### Option 1: Demo Mode (No Build Required)

The playground can run in demo mode with mock syntax highlighting:

1. Open `index.html` in a modern web browser
2. Select a language and theme from the dropdowns
3. Type or paste code in the editor
4. See the highlighted output on the right

The demo mode uses simple regex-based highlighting to demonstrate the UI without requiring WASM compilation.

### Option 2: Full Mode (with WASM)

For real TextMate syntax highlighting:

1. **Build the WASM module** (requires Emscripten):
   ```bash
   # From the project root
   ./scripts/build-wasm-standard.sh
   ```

2. **Copy WASM files to playground**:
   ```bash
   mkdir -p playground/wasm
   cp build/wasm-standard/browser/tml-standard.js playground/wasm/
   cp build/wasm-standard/browser/tml-standard.wasm playground/wasm/
   ```

3. **Serve the playground** (required for WASM to work):
   ```bash
   # Using Python 3
   cd playground
   python3 -m http.server 8000
   
   # Or using Node.js with http-server
   npx http-server playground -p 8000
   ```

4. **Open in browser**: Navigate to `http://localhost:8000`

## Directory Structure

```
playground/
├── index.html          # Main HTML file
├── styles.css          # Styling and layout
├── app.js              # Main application logic
├── grammars.js         # Grammar definitions and paths
├── themes.js           # Theme definitions and paths
├── wasm/               # WASM module files (after build)
│   ├── tml-standard.js
│   └── tml-standard.wasm
└── README.md           # This file
```

## Grammars and Themes

The playground uses grammars and themes from the [textmate-grammars-themes](https://github.com/shikijs/textmate-grammars-themes) repository, which is included as a submodule.

### Supported Languages

The playground includes support for 30+ languages:

- **Web**: JavaScript, TypeScript, HTML, CSS, JSX, TSX, Vue
- **Systems**: C, C++, Rust, Go
- **JVM**: Java, Kotlin, Scala
- **Scripting**: Python, Ruby, PHP, Lua, Bash
- **Functional**: Haskell, R
- **Mobile**: Swift
- **.NET**: C#
- **Data**: JSON, YAML, XML, SQL, GraphQL
- **Documentation**: Markdown
- **DevOps**: Dockerfile

### Available Themes

Choose from 40+ themes including:

- **Default**: Dark+, Light+
- **Popular**: Monokai, Dracula, One Dark Pro
- **GitHub**: GitHub Dark, GitHub Light
- **Modern**: Tokyo Night, Nord, Material Theme
- **Classic**: Solarized Dark/Light, Gruvbox variants
- **Aesthetic**: Catppuccin variants, Vitesse, Ayu, Everforest
- **Unique**: LaserWave, Vesper, Aurora X, Night Owl

## How It Works

### Architecture

1. **Grammar Loading**: Grammars are loaded from JSON files in the `textmate-grammars-themes` submodule
2. **Theme Application**: Themes define color schemes for different token scopes
3. **Tokenization**: Code is tokenized line-by-line using the selected grammar
4. **Rendering**: Tokens are rendered with colors from the selected theme
5. **Debug View**: Shows detailed information about each token's scope and color

### WASM Integration

When the WASM module is available:

1. The TextMateLib C++ library is compiled to WebAssembly
2. JavaScript bindings expose the tokenization API
3. The playground loads grammars and tokenizes code in real-time
4. Full TextMate syntax highlighting is applied with proper scope handling

### Demo Mode

When WASM is not available:

1. Simple regex-based highlighting is used
2. Keywords, strings, comments, and numbers are detected
3. Basic styling is applied to demonstrate the UI
4. Debug view shows simplified token information

## Usage Tips

### Selecting a Grammar

1. Use the "Language" dropdown to select a programming language
2. The editor will load with sample code for that language
3. You can override the sample by typing your own code

### Choosing a Theme

1. Use the "Theme" dropdown to select a color scheme
2. The output panel updates immediately with the new colors
3. Themes are grouped by type (dark/light)

### Using Debug View

1. Click "Toggle Debug View" to show/hide the debug panel
2. The debug view shows:
   - Line-by-line token breakdown
   - Token text and boundaries
   - Scope names for each token
   - Applied colors (when WASM is available)

### Editing Code

1. Type or paste code in the left editor panel
2. The highlighted output updates automatically
3. Syntax errors won't break the highlighting (graceful degradation)

## Development

### Adding New Grammars

1. Ensure the grammar exists in `thirdparty/textmate-grammars-themes/packages/tm-grammars/grammars/`
2. Add an entry to `grammars.js`:
   ```javascript
   {
       name: 'mylang',
       displayName: 'My Language',
       scopeName: 'source.mylang',
       path: `${BASE_PATH}/mylang.json`,
   }
   ```

### Adding New Themes

1. Ensure the theme exists in `thirdparty/textmate-grammars-themes/packages/tm-themes/themes/`
2. Add an entry to `themes.js`:
   ```javascript
   {
       name: 'mytheme',
       displayName: 'My Theme',
       path: `${BASE_PATH}/mytheme.json`,
       type: 'dark', // or 'light'
   }
   ```

### Customizing the UI

- Edit `styles.css` to change colors, layout, or animations
- Modify `index.html` to add new UI elements
- Update `app.js` to add new features or change behavior

## Troubleshooting

### WASM Module Not Loading

**Problem**: "WASM module not found" error

**Solution**:
1. Build the WASM module: `./scripts/build-wasm-standard.sh`
2. Copy files to `playground/wasm/` directory
3. Serve the playground with a local web server (file:// protocol won't work)

### Grammar Not Found

**Problem**: Selected grammar doesn't load

**Solution**:
1. Verify the grammar file exists in the submodule
2. Check the path in `grammars.js` matches the actual file location
3. Ensure the submodule is initialized: `git submodule update --init --recursive`

### Theme Colors Not Applying

**Problem**: Theme selected but colors don't change

**Solution**:
1. Verify the theme file exists in the submodule
2. Check browser console for JSON parsing errors
3. Ensure theme file is valid TextMate theme format

### Browser Compatibility

The playground requires:
- Modern browser with ES6 module support
- WebAssembly support (for full mode)
- Fetch API support

Tested browsers:
- ✅ Chrome 90+
- ✅ Firefox 88+
- ✅ Safari 14+
- ✅ Edge 90+

## Credits

- **TextMateLib**: C++ implementation of TextMate syntax highlighting
- **Grammars & Themes**: [textmate-grammars-themes](https://github.com/shikijs/textmate-grammars-themes) by the Shiki team
- **Original TextMate**: [TextMate](https://macromates.com/) by Allan Odgaard

## License

See the main project LICENSE file.
