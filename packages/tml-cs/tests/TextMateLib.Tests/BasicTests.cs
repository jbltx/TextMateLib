using System;
using TextMateLib.Bindings;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Basic smoke tests for the C# bindings
    /// </summary>
    public class BasicTests
    {
        [Fact]
        public void CanCreateRegistry()
        {
            // Arrange & Act
            using var registry = new Registry();

            // Assert
            Assert.NotNull(registry);
        }

        [Fact]
        public void CanLoadGrammarFromJson()
        {
            // Arrange
            var helloGrammar = @"{
                ""name"": ""hello"",
                ""scopeName"": ""source.hello"",
                ""fileTypes"": [""world""],
                ""patterns"": [
                    {
                        ""match"": ""hello"",
                        ""name"": ""prefix.hello""
                    }
                ]
            }";

            using var registry = new Registry();

            // Act
            registry.AddGrammarFromJson(helloGrammar);
            using var grammar = registry.LoadGrammar("source.hello");

            // Assert
            Assert.NotNull(grammar);
            Assert.Equal("source.hello", grammar.ScopeName);
        }

        [Fact]
        public void CanTokenizeWithSimpleGrammar()
        {
            // Arrange
            var helloGrammar = @"{
                ""name"": ""hello"",
                ""scopeName"": ""source.hello"",
                ""fileTypes"": [""world""],
                ""patterns"": [
                    {
                        ""match"": ""hello"",
                        ""name"": ""prefix.hello""
                    }
                ]
            }";

            using var registry = new Registry();
            registry.AddGrammarFromJson(helloGrammar);
            using var grammar = registry.LoadGrammar("source.hello");

            // Act
            var result = grammar.TokenizeLine("hello world", IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);
        }
    }
}
