# TextMateLib C++ API Documentation

This directory contains the Doxygen configuration for generating API documentation for the TextMateLib C++ library.

## Prerequisites

You need to have Doxygen installed on your system.

### Installation

**macOS:**
```bash
brew install doxygen
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install doxygen graphviz
```

**Windows:**
Download from [doxygen.nl](https://www.doxygen.nl/download.html)

## Building Documentation

To generate the documentation:

```bash
cd docs
doxygen Doxyfile
```

The HTML documentation will be generated in `docs/html/`. Open `docs/html/index.html` in a web browser to view it.

## Configuration

The `Doxyfile` is configured to:

- **Source**: Scans the `src/` directory recursively
- **Output**: Generates HTML documentation
- **Excludes**: WebAssembly and C# bindings (in `wasm/` and `csharp/` directories)
- **Features**:
  - Includes class inheritance diagrams
  - Shows call graphs and include dependencies
  - Extracts documentation from C++ comments (Doxygen/Javadoc style)
  - Supports Markdown in comments

## Documentation Standards

To get the best documentation generation, follow these comment conventions in your C++ code:

### Classes and Structs

```cpp
/// @brief A brief description of the class
///
/// Detailed description of the class, including usage examples,
/// important notes, or additional context.
class MyClass {
    // ...
};
```

### Functions and Methods

```cpp
/// @brief Brief description of what the function does
///
/// Detailed description (optional)
///
/// @param paramName Description of the parameter
/// @return Description of the return value
/// @throws std::exception Description of exceptions (if applicable)
///
/// @example
/// Basic usage example
void myFunction(int paramName);
```

### Member Variables

```cpp
/// Brief description of the member variable
int memberVar;
```

### Groups and Modules

```cpp
/// @defgroup Tokenization Tokenization Engine
/// @{
/// Group of related tokenization functions

/// @enddef
```

## Viewing Generated Docs

After building:

```bash
# Open in default browser (macOS)
open docs/html/index.html

# Or directly with a browser
firefox docs/html/index.html
# or
google-chrome docs/html/index.html
```

## Continuous Documentation

To regenerate documentation automatically during development:

```bash
# Watch for changes and regenerate (requires entr or similar)
find ../src -name "*.h" -o -name "*.cpp" | entr doxygen Doxyfile
```

## Key Source Files to Document

When adding documentation, prioritize these core modules:

1. **src/main.h** - Public C++ API
2. **src/types.h** - Core type definitions
3. **src/grammar.h** - Tokenization logic
4. **src/session.h** - Stateful editor session API
5. **src/registry.h** - Grammar/theme management
6. **src/theme.h** - Theme/styling support
7. **src/c_api.h** - C FFI bindings

## Customization

To customize the documentation output:

1. Edit the `Doxyfile` configuration
2. Common customizations:
   - `PROJECT_LOGO` - Add a project logo
   - `HTML_COLORSTYLE_*` - Change color scheme
   - `GENERATE_LATEX` - Enable PDF output
   - `GENERATE_MAN` - Enable man page generation

For full Doxyfile options, see the [Doxygen Manual](https://www.doxygen.nl/manual/config.html)

## Troubleshooting

### "doxygen: command not found"
Install Doxygen (see Prerequisites section above)

### Documentation not updating
Delete the `html/` directory and regenerate:
```bash
rm -rf html
doxygen Doxyfile
```

### Graphs not showing
Install Graphviz for better diagram generation:
```bash
# macOS
brew install graphviz

# Linux
sudo apt-get install graphviz
```
