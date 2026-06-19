using System;
using System.IO;
using TextMateLib.Bindings;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Tests for themed (encoded) tokenization: TokenizeLine2 / TokenizeLines2 and the
    /// registry color map. These exercise the encoded-token marshalling path
    /// (MarshalEncodedTokens) and GetColorMap, which the plain-token tests never reach.
    /// </summary>
    public class ThemedTokenizationTests : IDisposable
    {
        readonly Registry m_Registry;

        readonly Grammar m_JsGrammar;

        public ThemedTokenizationTests()
        {
            // Set up library path for Linux
            var nativeLibPath = Path.Combine(AppContext.BaseDirectory, "native");
            Environment.SetEnvironmentVariable("LD_LIBRARY_PATH",
                nativeLibPath + ":" + Environment.GetEnvironmentVariable("LD_LIBRARY_PATH"));

            var fixturesPath = Path.Combine(AppContext.BaseDirectory, "fixtures");

            m_Registry = new Registry();

            var jsGrammarPath = Path.Combine(fixturesPath, "grammars", "javascript-test.json");
            m_Registry.AddGrammarFromFile(jsGrammarPath);

            // A theme must be set on the registry before themed tokenization produces colors.
            var themePath = Path.Combine(fixturesPath, "themes", "github-dark-high-contrast.json");
            m_Registry.SetThemeFromJson(File.ReadAllText(themePath));

            m_JsGrammar = m_Registry.LoadGrammar("source.js");
        }

        [Fact]
        public void TokenizeLines2_ReturnsOneResultPerLine()
        {
            var lines = new[]
            {
                "const x = 42;",
                "let s = \"hello\";",
                "function f() { return x; }",
            };

            var results = m_JsGrammar.TokenizeLines2(lines);

            Assert.NotNull(results);
            Assert.Equal(lines.Length, results.Length);

            foreach (var result in results)
            {
                Assert.NotNull(result);
                Assert.NotEmpty(result.Tokens);
                // The first token of a line always starts at column 0.
                Assert.Equal(0, result.Tokens[0].StartIndex);
            }
        }

        [Fact]
        public void TokenizeLines2_EmptyOrNullInputReturnsEmpty()
        {
            Assert.Empty(m_JsGrammar.TokenizeLines2(Array.Empty<string>()));
            Assert.Empty(m_JsGrammar.TokenizeLines2(null!));
        }

        [Fact]
        public void GetColorMap_ReturnsNonEmptyMapAfterTheme()
        {
            var colorMap = m_Registry.GetColorMap();

            Assert.NotNull(colorMap);
            Assert.NotEmpty(colorMap);
        }

        [Fact]
        public void EncodedTokenColorIdsAreValidColorMapIndices()
        {
            var colorMap = m_Registry.GetColorMap();
            Assert.NotEmpty(colorMap);

            var results = m_JsGrammar.TokenizeLines2(new[] { "const x = 42;" });

            // Every decoded color id must index into the color map. Garbage metadata
            // (e.g. from a mis-sized copy buffer) would fall outside this range.
            foreach (var token in results[0].Tokens)
            {
                Assert.InRange(token.ForegroundId, 0, colorMap.Length - 1);
                Assert.InRange(token.BackgroundId, 0, colorMap.Length - 1);
            }
        }

        [Fact]
        public void TokenizeLine2_MatchesSingleLineBatch()
        {
            const string line = "function f() { return 'x'; }";

            // Both start from the initial state on the same input, so the single-line and
            // batch marshalling paths must yield byte-identical encoded tokens.
            var single = m_JsGrammar.TokenizeLine2(line, IntPtr.Zero);
            var batch = m_JsGrammar.TokenizeLines2(new[] { line });

            Assert.Single(batch);

            var batchTokens = batch[0].Tokens;
            Assert.Equal(single.TokenCount, batchTokens.Count);

            for (int i = 0; i < single.TokenCount; i++)
            {
                Assert.Equal(single.Tokens[i].StartIndex, batchTokens[i].StartIndex);
                Assert.Equal(single.Tokens[i].Metadata, batchTokens[i].Metadata);
            }
        }

        public void Dispose()
        {
            m_JsGrammar?.Dispose();
            m_Registry?.Dispose();
        }
    }
}
