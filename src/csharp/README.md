# TextMateLib C# Bindings

.NET 9 bindings for the TextMateLib syntax highlighting library.

## Overview

This library provides C# bindings to TextMateLib, enabling syntax highlighting and theme support for .NET applications. The bindings use P/Invoke to interface with the native TextMateLib C API.

## Features

- ✅ **Theme Support**: Load and apply TextMate themes to get colors and font styles for scopes
- ✅ **Registry Management**: Register and manage multiple grammars
- ⚠️ **Grammar Loading**: Grammar registration (tokenization support in progress)
- ✅ **Memory Safety**: Automatic disposal of native resources via IDisposable pattern

## Building

### Prerequisites

- .NET 9 SDK
- Native TextMateLib shared library (libtml.so on Linux, libtml.dylib on macOS, tml.dll on Windows)

### Build Instructions

```bash
# Build the native library first
cd /path/to/TextMateLib
./scripts/build-shared.sh

# Build the C# bindings
cd src/csharp/TextMateLib.Bindings
dotnet build

# Run tests
cd ../../../tests/csharp/TextMateLib.Tests
dotnet test
```

## Usage

### Loading and Using Themes

```csharp
using TextMateLib.Bindings;

// Load a theme from a JSON file
using var theme = Theme.LoadFromFile("path/to/theme.json");

// Get colors for a scope
uint foreground = theme.GetForeground("keyword.control", 0xFFFFFFFF);
uint background = theme.GetBackground("keyword.control", 0x00000000);
FontStyle style = theme.GetFontStyle("keyword.control", FontStyle.None);

// Get default theme colors
uint defaultFg = theme.GetDefaultForeground();
uint defaultBg = theme.GetDefaultBackground();
```

### Loading Themes from JSON String

```csharp
var themeJson = @"{
    ""name"": ""My Theme"",
    ""type"": ""dark"",
    ""colors"": {
        ""editor.foreground"": ""#FFFFFF"",
        ""editor.background"": ""#000000""
    },
    ""tokenColors"": [
        {
            ""scope"": ""keyword"",
            ""settings"": {
                ""foreground"": ""#FF0000"",
                ""fontStyle"": ""bold""
            }
        }
    ]
}";

using var theme = Theme.LoadFromJson(themeJson);
```

### Registry and Grammar Management

```csharp
using TextMateLib.Bindings;

// Create a registry
using var registry = new Registry();

// Add grammars to the registry
registry.AddGrammarFromFile("path/to/javascript.json");
registry.AddGrammarFromJson(grammarJsonString);

// Load a grammar by scope name
using var grammar = registry.LoadGrammar("source.js");
```

## API Reference

### Theme Class

#### Methods

- `static Theme LoadFromFile(string themePath)`: Load theme from a JSON file
- `static Theme LoadFromJson(string jsonContent)`: Load theme from a JSON string
- `uint GetForeground(string scopePath, uint defaultColor)`: Get foreground color for a scope
- `uint GetBackground(string scopePath, uint defaultColor)`: Get background color for a scope
- `FontStyle GetFontStyle(string scopePath, FontStyle defaultStyle)`: Get font style for a scope
- `uint GetDefaultForeground()`: Get the default foreground color
- `uint GetDefaultBackground()`: Get the default background color

#### Color Format

Colors are returned as `uint` in RGBA format: `0xRRGGBBAA`
- RR: Red component (0-255)
- GG: Green component (0-255)
- BB: Blue component (0-255)
- AA: Alpha component (0-255)

#### Font Styles

```csharp
[Flags]
public enum FontStyle
{
    None = 0,
    Italic = 1,
    Bold = 2,
    Underline = 4
}
```

### Registry Class

#### Methods

- `void AddGrammarFromFile(string grammarPath)`: Add a grammar from a JSON file
- `void AddGrammarFromJson(string jsonContent)`: Add a grammar from a JSON string
- `Grammar LoadGrammar(string scopeName)`: Load a grammar by its scope name

### Grammar Class

#### Methods

- `string ScopeName`: Get the scope name of the grammar
- `TokenizeResult TokenizeLine(string lineText, IntPtr prevState)`: Tokenize a single line
- `TokenizeResult[] TokenizeLines(string[] lines)`: Tokenize multiple lines

## Project Structure

```
src/csharp/
├── TextMateLib.Bindings/
│   ├── NativeMethods.cs      # P/Invoke declarations
│   ├── Theme.cs               # Theme wrapper class
│   ├── Registry.cs            # Registry wrapper class
│   ├── Grammar.cs             # Grammar wrapper class
│   ├── Token.cs               # Token data structures
│   └── TextMateLib.Bindings.csproj

tests/csharp/
└── TextMateLib.Tests/
    ├── ThemeTests.cs          # Theme functionality tests
    ├── TokenizationTests.cs   # Tokenization tests (in progress)
    ├── BasicTests.cs          # Basic API tests
    └── TextMateLib.Tests.csproj
```

## Testing

The test suite uses xUnit and includes:

- **Theme Tests**: 11 tests covering theme loading and color/style retrieval - ✅ All passing
- **Basic Tests**: Registry creation and management
- **Tokenization Tests**: Grammar loading and tokenization (in development)

Run tests with:

```bash
cd tests/csharp/TextMateLib.Tests
dotnet test
```

## Known Issues and Limitations

1. **Grammar Tokenization**: Grammar loading and tokenization are functional at the C API level, but string marshaling for scope names needs additional work.
2. **Native Library Deployment**: The native library (libtml.so/dylib/dll) must be available in the application's library path or copied to the output directory.

## Future Enhancements

- Complete tokenization API implementation
- Add Session API bindings for incremental tokenization
- Add Syntax Highlighter API bindings
- NuGet package distribution
- Improved native library deployment

## License

Same as TextMateLib parent project.
