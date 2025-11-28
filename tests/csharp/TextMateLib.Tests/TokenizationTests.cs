using System;
using System.IO;
using TextMateLib.Bindings;
using Xunit;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Tests for basic tokenization functionality
    /// </summary>
    public class TokenizationTests : IDisposable
    {
        private readonly string _fixturesPath;
        private readonly Registry _registry;
        private readonly Grammar _jsGrammar;
        private readonly Grammar _pythonGrammar;

        public TokenizationTests()
        {
            // Set up library path for Linux
            var nativeLibPath = Path.Combine(AppContext.BaseDirectory, "native");
            Environment.SetEnvironmentVariable("LD_LIBRARY_PATH", 
                nativeLibPath + ":" + Environment.GetEnvironmentVariable("LD_LIBRARY_PATH"));

            _fixturesPath = Path.Combine(AppContext.BaseDirectory, "fixtures");
            
            _registry = new Registry();
            
            // Load JavaScript grammar
            var jsGrammarPath = Path.Combine(_fixturesPath, "grammars", "javascript-test.json");
            _registry.AddGrammarFromFile(jsGrammarPath);
            _jsGrammar = _registry.LoadGrammar("source.js");
            
            // Load Python grammar
            var pythonGrammarPath = Path.Combine(_fixturesPath, "grammars", "python-test.json");
            _registry.AddGrammarFromFile(pythonGrammarPath);
            _pythonGrammar = _registry.LoadGrammar("source.python");
        }

        [Fact]
        public void CanTokenizeSimpleJavaScriptLine()
        {
            // Arrange
            var code = "const x = 42;";

            // Act
            var result = _jsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);
            Assert.False(result.StoppedEarly);
            
            // Verify at least some tokens exist
            Assert.True(result.Tokens.Count > 0, "Expected at least one token");
        }

        [Fact]
        public void CanTokenizeJavaScriptFunction()
        {
            // Arrange
            var code = "function hello() { return 'world'; }";

            // Act
            var result = _jsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);
            
            // Check that we have tokens with different scopes
            bool foundKeyword = false;
            bool foundString = false;
            
            foreach (var token in result.Tokens)
            {
                var scopeStr = string.Join(" ", token.Scopes);
                if (scopeStr.Contains("keyword"))
                    foundKeyword = true;
                if (scopeStr.Contains("string"))
                    foundString = true;
            }
            
            Assert.True(foundKeyword, "Expected to find keyword token");
            Assert.True(foundString, "Expected to find string token");
        }

        [Fact]
        public void CanTokenizeMultipleLines()
        {
            // Arrange
            var lines = new[]
            {
                "function test() {",
                "  const x = 10;",
                "  return x;",
                "}"
            };

            // Act
            var results = _jsGrammar.TokenizeLines(lines);

            // Assert
            Assert.NotNull(results);
            Assert.Equal(lines.Length, results.Length);
            
            foreach (var result in results)
            {
                Assert.NotNull(result);
                Assert.NotEmpty(result.Tokens);
            }
        }

        [Fact]
        public void CanTokenizePythonCode()
        {
            // Arrange
            var code = "def hello():\n    print('world')";
            var lines = code.Split('\n');

            // Act
            var results = _pythonGrammar.TokenizeLines(lines);

            // Assert
            Assert.NotNull(results);
            Assert.Equal(lines.Length, results.Length);
            
            // First line should have "def" keyword
            bool foundDefKeyword = false;
            foreach (var token in results[0].Tokens)
            {
                var scopeStr = string.Join(" ", token.Scopes);
                if (scopeStr.Contains("keyword") && scopeStr.Contains("function"))
                    foundDefKeyword = true;
            }
            Assert.True(foundDefKeyword, "Expected to find 'def' keyword in first line");
        }

        [Fact]
        public void TokensHaveCorrectPositions()
        {
            // Arrange
            var code = "let x = 5;";

            // Act
            var result = _jsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);
            
            // Verify that tokens cover the entire line
            var firstToken = result.Tokens[0];
            Assert.Equal(0, firstToken.StartIndex);
            
            // Verify tokens don't overlap and are in order
            for (int i = 0; i < result.Tokens.Count - 1; i++)
            {
                Assert.True(result.Tokens[i].EndIndex <= result.Tokens[i + 1].StartIndex,
                    "Tokens should not overlap");
            }
        }

        [Fact]
        public void TokenCanExtractValue()
        {
            // Arrange
            var code = "const hello = 'world';";

            // Act
            var result = _jsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);
            
            // Find the 'const' keyword token
            bool foundConst = false;
            foreach (var token in result.Tokens)
            {
                var value = token.GetValue(code);
                if (value == "const")
                {
                    foundConst = true;
                    break;
                }
            }
            
            Assert.True(foundConst, "Expected to find 'const' token");
        }

        [Fact]
        public void GrammarHasCorrectScopeName()
        {
            // Assert
            Assert.Equal("source.js", _jsGrammar.ScopeName);
            Assert.Equal("source.python", _pythonGrammar.ScopeName);
        }

        [Fact]
        public void StateStackMaintainedBetweenLines()
        {
            // Arrange - multi-line string in JavaScript
            var lines = new[]
            {
                "const text = `",
                "  multi-line",
                "  string",
                "`;",
            };

            // Act
            var results = _jsGrammar.TokenizeLines(lines);

            // Assert
            Assert.NotNull(results);
            Assert.Equal(lines.Length, results.Length);
            
            // Each line should have a state (except the first might use initial state)
            for (int i = 0; i < results.Length; i++)
            {
                Assert.NotEqual(IntPtr.Zero, results[i].StateStack);
            }
        }

        public void Dispose()
        {
            _jsGrammar?.Dispose();
            _pythonGrammar?.Dispose();
            _registry?.Dispose();
        }
    }
}
