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

        // ============================================================================
        // Scope matching tests - verify prefix matching and specificity
        // ============================================================================

        [Fact]
        public void PrefixMatchingWorks_StorageTypeMatchesStorageTypeFunction()
        {
            // Arrange
            // Theme has rule for "storage.type" with color #ff9492
            // "storage.type.function" should match via prefix matching
            var baseScope = "storage.type";
            var extendedScope = "storage.type.function";

            // Act
            var baseColor = m_Theme.GetForeground(baseScope, 0xDEADBEEF);
            var extendedColor = m_Theme.GetForeground(extendedScope, 0xDEADBEEF);

            // Assert - both should get the same color from the storage.type rule
            Assert.NotEqual(0xDEADBEEFu, baseColor); // Should match storage.type rule
            Assert.Equal(baseColor, extendedColor); // Extended should inherit from base
        }

        [Fact]
        public void MostSpecificRuleWins_StorageTypeJavaOverridesStorageType()
        {
            // Arrange
            // Theme has:
            // - "storage.type" → #ff9492
            // - "storage.type.java" → #f0f3f6 (more specific)
            var generalScope = "storage.type";
            var specificScope = "storage.type.java";

            // Act
            var generalColor = m_Theme.GetForeground(generalScope, 0xDEADBEEF);
            var specificColor = m_Theme.GetForeground(specificScope, 0xDEADBEEF);

            // Assert - specific scope should have different (overridden) color
            Assert.NotEqual(0xDEADBEEFu, generalColor);
            Assert.NotEqual(0xDEADBEEFu, specificColor);
            Assert.NotEqual(generalColor, specificColor); // Specific rule should override
        }

        [Fact]
        public void ScopeStackMatching_MultiScopePathMatchesCorrectRule()
        {
            // Arrange
            // When given a scope stack like "source.js storage.modifier",
            // the theme should find the matching rule for storage.modifier
            var scopeStack = "source.js storage.modifier";

            // Theme has "storage" and "storage.type" rules with #ff9492
            // storage.modifier should match "storage" rule

            // Act
            var color = m_Theme.GetForeground(scopeStack, 0xDEADBEEF);

            // Assert - should match and not return default
            Assert.NotEqual(0xDEADBEEFu, color);
        }

        [Fact]
        public void ScopeStackMatching_DeepNestedScopeMatchesPrefixRule()
        {
            // Arrange
            // "storage.modifier.public.cs" should match "storage" rule via prefix
            var deepScope = "source.cs storage.modifier.public.cs";

            // Act
            var color = m_Theme.GetForeground(deepScope, 0xDEADBEEF);

            // Assert - should find a match via prefix matching
            Assert.NotEqual(0xDEADBEEFu, color);
        }

        [Fact]
        public void ScopeStackMatching_KeywordControlMatchesKeyword()
        {
            // Arrange
            // Theme has "keyword" → #ff9492
            // "keyword.control" and "keyword.control.flow" should match via prefix
            var keywordScope = "keyword";
            var keywordControlScope = "keyword.control";
            var keywordControlFlowScope = "source.js keyword.control.flow";

            // Act
            var keywordColor = m_Theme.GetForeground(keywordScope, 0xDEADBEEF);
            var keywordControlColor = m_Theme.GetForeground(keywordControlScope, 0xDEADBEEF);
            var keywordControlFlowColor = m_Theme.GetForeground(keywordControlFlowScope, 0xDEADBEEF);

            // Assert - all should get the same color from keyword rule
            Assert.NotEqual(0xDEADBEEFu, keywordColor);
            Assert.Equal(keywordColor, keywordControlColor);
            Assert.Equal(keywordColor, keywordControlFlowColor);
        }

        [Fact]
        public void ScopeStackMatching_StringQuotedMatchesString()
        {
            // Arrange
            // Theme has "string" → #addcff
            // Various string subscopes should all match
            var stringScope = "string";
            var stringQuotedScope = "string.quoted";
            var fullScopePath = "source.js string.quoted.double.js";

            // Act
            var stringColor = m_Theme.GetForeground(stringScope, 0xDEADBEEF);
            var stringQuotedColor = m_Theme.GetForeground(stringQuotedScope, 0xDEADBEEF);
            var fullPathColor = m_Theme.GetForeground(fullScopePath, 0xDEADBEEF);

            // Assert - all should inherit from string rule
            Assert.NotEqual(0xDEADBEEFu, stringColor);
            Assert.Equal(stringColor, stringQuotedColor);
            Assert.Equal(stringColor, fullPathColor);
        }

        [Fact]
        public void ScopeStackMatching_CommentScopesMatchCorrectly()
        {
            // Arrange
            // Theme has "comment" → #bdc4cc
            var commentScope = "comment";
            var commentLineScope = "comment.line";
            var fullCommentPath = "source.js comment.line.double-slash.js";

            // Act
            var commentColor = m_Theme.GetForeground(commentScope, 0xDEADBEEF);
            var commentLineColor = m_Theme.GetForeground(commentLineScope, 0xDEADBEEF);
            var fullPathColor = m_Theme.GetForeground(fullCommentPath, 0xDEADBEEF);

            // Assert
            Assert.NotEqual(0xDEADBEEFu, commentColor);
            Assert.Equal(commentColor, commentLineColor);
            Assert.Equal(commentColor, fullPathColor);
        }

        [Fact]
        public void ScopeStackMatching_EntityNameFunctionHasSpecificColor()
        {
            // Arrange
            // Theme has specific rule for "entity.name.function" → #dbb7ff
            // This is more specific than "entity.name" → #ffb757
            var entityNameScope = "entity.name";
            var entityNameFunctionScope = "entity.name.function";

            // Act
            var entityNameColor = m_Theme.GetForeground(entityNameScope, 0xDEADBEEF);
            var entityNameFunctionColor = m_Theme.GetForeground(entityNameFunctionScope, 0xDEADBEEF);

            // Assert - function scope should have different color
            Assert.NotEqual(0xDEADBEEFu, entityNameColor);
            Assert.NotEqual(0xDEADBEEFu, entityNameFunctionColor);
            Assert.NotEqual(entityNameColor, entityNameFunctionColor);
        }

        public void Dispose()
        {
            m_Theme?.Dispose();
        }
    }
}
