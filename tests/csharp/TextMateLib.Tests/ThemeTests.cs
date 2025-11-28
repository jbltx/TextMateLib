using System;
using System.IO;
using TextMateLib.Bindings;
using Xunit;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Tests for theme application functionality
    /// </summary>
    public class ThemeTests : IDisposable
    {
        private readonly string _fixturesPath;
        private readonly Theme _theme;

        public ThemeTests()
        {
            // Set up library path for Linux
            var nativeLibPath = Path.Combine(AppContext.BaseDirectory, "native");
            Environment.SetEnvironmentVariable("LD_LIBRARY_PATH", 
                nativeLibPath + ":" + Environment.GetEnvironmentVariable("LD_LIBRARY_PATH"));

            _fixturesPath = Path.Combine(AppContext.BaseDirectory, "fixtures");
            
            // Load theme
            var themePath = Path.Combine(_fixturesPath, "themes", "github-dark-high-contrast.json");
            _theme = Theme.LoadFromFile(themePath);
        }

        [Fact]
        public void CanLoadThemeFromFile()
        {
            // Assert
            Assert.NotNull(_theme);
        }

        [Fact]
        public void ThemeHasDefaultColors()
        {
            // Act
            var foreground = _theme.GetDefaultForeground();
            var background = _theme.GetDefaultBackground();

            // Assert
            Assert.NotEqual(0u, foreground);
            // Background could be 0 (transparent), so we just check it doesn't throw
        }

        [Fact]
        public void CanGetForegroundColorForScope()
        {
            // Arrange
            var scope = "keyword.control";

            // Act
            var color = _theme.GetForeground(scope, 0xFFFFFFFF);

            // Assert
            Assert.NotEqual(0u, color);
        }

        [Fact]
        public void CanGetBackgroundColorForScope()
        {
            // Arrange
            var scope = "string.quoted";

            // Act
            var color = _theme.GetBackground(scope, 0x00000000);

            // Assert - background might be transparent (0), which is valid
            // Just ensure no exception is thrown
        }

        [Fact]
        public void CanGetFontStyleForScope()
        {
            // Arrange
            var commentScope = "comment";

            // Act
            var style = _theme.GetFontStyle(commentScope, FontStyle.None);

            // Assert
            // Comments are often italic in themes, but we just verify it returns something valid
            Assert.True(Enum.IsDefined(typeof(FontStyle), style) || 
                       ((int)style & ~0xF) == 0, // Check it's within valid flag range
                       "Font style should be a valid FontStyle value");
        }

        [Fact]
        public void FontStyleFlagsWork()
        {
            // Test that font style flags can be combined
            FontStyle combined = FontStyle.Bold | FontStyle.Italic;
            
            Assert.True((combined & FontStyle.Bold) == FontStyle.Bold);
            Assert.True((combined & FontStyle.Italic) == FontStyle.Italic);
            Assert.False((combined & FontStyle.Underline) == FontStyle.Underline);
        }

        [Fact]
        public void DefaultColorIsReturnedForUnknownScope()
        {
            // Arrange
            var unknownScope = "this.is.definitely.not.a.real.scope.name.xyz123";
            var defaultColor = 0xDEADBEEFu;

            // Act
            var color = _theme.GetForeground(unknownScope, defaultColor);

            // Assert
            Assert.Equal(defaultColor, color);
        }

        [Fact]
        public void CanLoadThemeFromJson()
        {
            // Arrange
            var minimalTheme = @"{
                ""name"": ""Test Theme"",
                ""type"": ""dark"",
                ""colors"": {
                    ""editor.foreground"": ""#FFFFFF"",
                    ""editor.background"": ""#000000""
                },
                ""tokenColors"": [
                    {
                        ""scope"": ""keyword"",
                        ""settings"": {
                            ""foreground"": ""#FF0000""
                        }
                    }
                ]
            }";

            // Act
            using var theme = Theme.LoadFromJson(minimalTheme);

            // Assert
            Assert.NotNull(theme);
            var foreground = theme.GetDefaultForeground();
            Assert.NotEqual(0u, foreground);
        }

        [Fact]
        public void ThemeIntegrationWithTokenization()
        {
            // Arrange
            var registry = new Registry();
            var jsGrammarPath = Path.Combine(_fixturesPath, "grammars", "javascript.json");
            registry.AddGrammarFromFile(jsGrammarPath);
            var grammar = registry.LoadGrammar("source.js");
            
            var code = "const x = 'hello';";

            // Act
            var result = grammar.TokenizeLine(code, IntPtr.Zero);

            // Assert - verify we can get colors for tokenized scopes
            Assert.NotNull(result);
            foreach (var token in result.Tokens)
            {
                if (token.Scopes.Count > 0)
                {
                    var scopePath = string.Join(" ", token.Scopes);
                    var color = _theme.GetForeground(scopePath, 0xFFFFFFFF);
                    
                    // Should return a valid color (not 0, as we provided non-zero default)
                    Assert.NotEqual(0u, color);
                }
            }

            grammar.Dispose();
            registry.Dispose();
        }

        [Fact]
        public void DisposingThemeTwiceDoesNotCrash()
        {
            // Arrange
            var themePath = Path.Combine(_fixturesPath, "themes", "github-dark-high-contrast.json");
            var theme = Theme.LoadFromFile(themePath);

            // Act & Assert - should not throw
            theme.Dispose();
            theme.Dispose();
        }

        [Fact]
        public void AccessingDisposedThemeThrows()
        {
            // Arrange
            var themePath = Path.Combine(_fixturesPath, "themes", "github-dark-high-contrast.json");
            var theme = Theme.LoadFromFile(themePath);
            theme.Dispose();

            // Act & Assert
            Assert.Throws<ObjectDisposedException>(() => theme.GetDefaultForeground());
        }

        public void Dispose()
        {
            _theme?.Dispose();
        }
    }
}
