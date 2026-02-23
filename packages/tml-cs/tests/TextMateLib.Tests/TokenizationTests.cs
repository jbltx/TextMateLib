using System;
using System.Linq;
using TextMateLib.Bindings;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Tests for basic tokenization functionality
    /// </summary>
    public class TokenizationTests : IDisposable
    {
        readonly string m_FixturesPath;

        readonly Registry m_Registry;

        readonly Grammar m_JsGrammar;

        readonly Grammar m_PythonGrammar;

        public TokenizationTests()
        {
            // Set up library path for Linux
            var nativeLibPath = Path.Combine(AppContext.BaseDirectory, "native");
            Environment.SetEnvironmentVariable("LD_LIBRARY_PATH",
                nativeLibPath + ":" + Environment.GetEnvironmentVariable("LD_LIBRARY_PATH"));

            m_FixturesPath = Path.Combine(AppContext.BaseDirectory, "fixtures");

            m_Registry = new Registry();

            // Load JavaScript grammar
            var jsGrammarPath = Path.Combine(m_FixturesPath, "grammars", "javascript-test.json");
            m_Registry.AddGrammarFromFile(jsGrammarPath);
            m_JsGrammar = m_Registry.LoadGrammar("source.js");

            // Load Python grammar
            var pythonGrammarPath = Path.Combine(m_FixturesPath, "grammars", "python-test.json");
            m_Registry.AddGrammarFromFile(pythonGrammarPath);
            m_PythonGrammar = m_Registry.LoadGrammar("source.python");
        }

        [Fact]
        public void CanTokenizeSimpleJavaScriptLine()
        {
            // Arrange
            var code = "const x = 42;";

            // Act
            var result = m_JsGrammar.TokenizeLine(code, IntPtr.Zero);

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
            var result = m_JsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);

            // Check that we have tokens with different scopes
            var foundKeyword = false;
            var foundString = false;

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
            var results = m_JsGrammar.TokenizeLines(lines);

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
            var results = m_PythonGrammar.TokenizeLines(lines);

            // Assert
            Assert.NotNull(results);
            Assert.Equal(lines.Length, results.Length);

            // First line should have "def" keyword
            Assert.True(results[0].Tokens.Count > 0, "Expected tokens in first line");
            Assert.Equal(3, results[0].Tokens[0].EndIndex);
            var foundDef = false;
            foreach (var scope in results[0].Tokens[0].Scopes)
            {
                if (scope == "storage.type.function.python")
                {
                    foundDef = true;
                    break;
                }
            }
            Assert.True(foundDef, "Expected to find 'def' keyword token");
        }

        [Fact]
        public void TokensHaveCorrectPositions()
        {
            // Arrange
            var code = "let x = 5;";

            // Act
            var result = m_JsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);

            // Verify that tokens cover the entire line
            var firstToken = result.Tokens[0];
            Assert.Equal(0, firstToken.StartIndex);

            // Verify tokens don't overlap and are in order
            for (var i = 0; i < result.Tokens.Count - 1; i++)
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
            var result = m_JsGrammar.TokenizeLine(code, IntPtr.Zero);

            // Assert
            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);

            // Find the 'const' keyword token
            var foundConst = false;
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
            Assert.Equal("source.js", m_JsGrammar.ScopeName);
            Assert.Equal("source.python", m_PythonGrammar.ScopeName);
        }

        [Fact]
        public void TokenIndicesAlignForNonAsciiCharacters()
        {
            // Emoji (U+1F600) = 4 UTF-8 bytes, 2 UTF-16 chars (surrogate pair).
            // If the native library returns UTF-8 byte offsets and they are used
            // directly as char indices, tokens after the emoji will misalign.
            var code = "\U0001F600 const x = 42;";

            var result = m_JsGrammar.TokenizeLine(code, IntPtr.Zero);

            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);

            // Verify tokens can extract their values without throwing
            // and that the full line is reconstructable from token values
            var reconstructed = string.Concat(
                result.Tokens.Select(t => t.GetValue(code)));

            Assert.Equal(code, reconstructed);

            // Verify the "const" keyword is found as a distinct token value
            var foundConst = result.Tokens.Any(t => t.GetValue(code) == "const");
            Assert.True(foundConst, "Expected to find 'const' token after emoji");
        }

        [Fact]
        public void TokenIndicesAlignForAccentedCharacters()
        {
            // Accented chars like 'é' = 2 UTF-8 bytes, 1 UTF-16 char.
            var code = "const café = 42;";

            var result = m_JsGrammar.TokenizeLine(code, IntPtr.Zero);

            Assert.NotNull(result);
            Assert.NotEmpty(result.Tokens);

            var reconstructed = string.Concat(
                result.Tokens.Select(t => t.GetValue(code)));

            Assert.Equal(code, reconstructed);
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
            var results = m_JsGrammar.TokenizeLines(lines);

            // Assert
            Assert.NotNull(results);
            Assert.Equal(lines.Length, results.Length);

            // Each line should have a state (except the first might use initial state)
            for (var i = 0; i < results.Length; i++)
            {
                Assert.NotEqual(IntPtr.Zero, results[i].StateStack);
            }
        }

        public void Dispose()
        {
            m_JsGrammar?.Dispose();
            m_PythonGrammar?.Dispose();
            m_Registry?.Dispose();
        }
    }
}
