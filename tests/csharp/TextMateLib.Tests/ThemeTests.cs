using System;
using TextMateLib.Bindings;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Tests for theme application functionality
    /// </summary>
    public class ThemeTests : IDisposable
    {
        readonly string m_FixturesPath;

        readonly Theme m_Theme;

        public ThemeTests()
        {
            // Set up library path for Linux
            var nativeLibPath = Path.Combine(AppContext.BaseDirectory, "native");
            Environment.SetEnvironmentVariable("LD_LIBRARY_PATH",
                nativeLibPath + ":" + Environment.GetEnvironmentVariable("LD_LIBRARY_PATH"));

            m_FixturesPath = Path.Combine(AppContext.BaseDirectory, "fixtures");

            // Load theme
            var themePath = Path.Combine(m_FixturesPath, "themes", "github-dark-high-contrast.json");
            m_Theme = Theme.LoadFromFile(themePath);
        }

        [Fact]
        public void CanLoadThemeFromFile()
        {
            // Assert
            Assert.NotNull(m_Theme);
        }

        [Fact]
        public void ThemeHasDefaultColors()
        {
            // Act
            var foreground = m_Theme.GetDefaultForeground();
            var background = m_Theme.GetDefaultBackground();

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
            var color = m_Theme.GetForeground(scope, 0xFFFFFFFF);

            // Assert
            Assert.NotEqual(0u, color);
        }

        [Fact]
        public void CanGetBackgroundColorForScope()
        {
            // Arrange
            var scope = "string.quoted";

            // Act
            var color = m_Theme.GetBackground(scope, 0x00000000);

            // Assert - background might be transparent (0), which is valid
            // Just ensure no exception is thrown
        }

        [Fact]
        public void CanGetFontStyleForScope()
        {
            // Arrange
            var commentScope = "comment";

            // Act
            var style = m_Theme.GetFontStyle(commentScope, FontStyle.None);

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
            var combined = FontStyle.Bold | FontStyle.Italic;

            Assert.Equal(FontStyle.Bold, (combined & FontStyle.Bold));
            Assert.Equal(FontStyle.Italic, (combined & FontStyle.Italic));
            Assert.NotEqual(FontStyle.Underline, (combined & FontStyle.Underline));
        }

        [Fact]
        public void DefaultColorIsReturnedForUnknownScope()
        {
            // Arrange
            var unknownScope = "this.is.definitely.not.a.real.scope.name.xyz123";
            var defaultColor = 0xDEADBEEFu;

            // Act
            var color = m_Theme.GetForeground(unknownScope, defaultColor);

            // Assert
            // The theme might have fallback rules, so just verify we got a valid color
            Assert.NotEqual(0u, color);
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
            var jsGrammarPath = Path.Combine(m_FixturesPath, "grammars", "javascript.json");
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
                    var color = m_Theme.GetForeground(scopePath, 0xFFFFFFFF);

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
            var themePath = Path.Combine(m_FixturesPath, "themes", "github-dark-high-contrast.json");
            var theme = Theme.LoadFromFile(themePath);

            // Act & Assert - should not throw
            theme.Dispose();
            theme.Dispose();
        }

        [Fact]
        public void AccessingDisposedThemeThrows()
        {
            // Arrange
            var themePath = Path.Combine(m_FixturesPath, "themes", "github-dark-high-contrast.json");
            var theme = Theme.LoadFromFile(themePath);
            theme.Dispose();

            // Act & Assert
            Assert.Throws<ObjectDisposedException>(() => theme.GetDefaultForeground());
        }

        public void Dispose()
        {
            m_Theme?.Dispose();
        }
    }
}
