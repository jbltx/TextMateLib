using System;
using System.IO;
using TextMateLib.Bindings;

namespace TextMateLib.Tests
{
    /// <summary>
    /// Grammar and theme payloads cross a P/Invoke boundary, where a parser that crashes on
    /// malformed input takes the process down instead of returning a failure code. These tests lock
    /// in that malformed input is rejected in managed code, before the native call.
    /// </summary>
    public class TextMateJsonTests
    {
        [Theory]
        [InlineData("{}")]
        [InlineData("{\"a\":1}")]
        [InlineData("{\"a\":[1,2,{\"b\":null}],\"c\":true,\"d\":-1.5e10}")]
        [InlineData("  {\"a\":\"\\u00e9\\n\"}  ")]
        public void IsWellFormedObject_AcceptsValidObjects(string json)
        {
            Assert.True(TextMateJson.IsWellFormedObject(json));
        }

        [Theory]
        [InlineData(null)]
        [InlineData("")]
        [InlineData("{")]
        [InlineData("{\"a\":}")]
        [InlineData("{\"a\":1,}")]
        [InlineData("{'a':1}")]
        [InlineData("[1,2]")]
        [InlineData("{} trailing")]
        [InlineData("{\"a\":01}")]
        [InlineData("{\"a\":\"unterminated}")]
        [InlineData("{\"a\":\"bad\\xescape\"}")]
        public void IsWellFormedObject_RejectsMalformedInput(string? json)
        {
            Assert.False(TextMateJson.IsWellFormedObject(json));
        }

        [Fact]
        public void IsWellFormedObject_RejectsExcessiveNesting()
        {
            var json = new string('[', 200);
            Assert.False(TextMateJson.IsWellFormedObject("{\"a\":" + json));
        }

        [Fact]
        public void Validate_ThrowsOnNullOrEmpty()
        {
            Assert.Throws<ArgumentNullException>(() => TextMateJson.Validate(null, "p", "grammar"));
            Assert.Throws<ArgumentNullException>(() => TextMateJson.Validate("", "p", "grammar"));
        }

        [Fact]
        public void Validate_ThrowsOnMalformedJson()
        {
            // InvalidOperationException, not ArgumentException: these entry points have always
            // thrown that for content that failed to load, and the validation moving earlier must
            // not change the exception type public callers already catch.
            Assert.Throws<InvalidOperationException>(() => TextMateJson.Validate("{\"a\":", "p", "grammar"));
        }

        [Fact]
        public void ResolveExistingFile_ThrowsWhenMissing()
        {
            Assert.Throws<ArgumentNullException>(() => TextMateJson.ResolveExistingFile(null, "p", "grammar"));
            Assert.Throws<InvalidOperationException>(
                () => TextMateJson.ResolveExistingFile(Path.Combine(Path.GetTempPath(), "no-such-grammar.json"), "p", "grammar"));
        }

        [Fact]
        public void ReadAndValidate_MalformedFile_ThrowsTheDocumentedLoadException()
        {
            var path = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName() + ".json");
            File.WriteAllText(path, "{ this is not valid json }");
            try
            {
                Assert.Throws<InvalidOperationException>(
                    () => TextMateJson.ReadAndValidate(path, "p", "grammar"));
            }
            finally
            {
                File.Delete(path);
            }
        }

        [Fact]
        public void ReadAndValidate_ReturnsTheContentItValidated()
        {
            // The content is returned so the caller can pass the validated bytes to the native
            // loader rather than a path the native side would re-read — the window in which the
            // file could change between the two reads is what this closes.
            var path = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName() + ".json");
            File.WriteAllText(path, "{\"scopeName\":\"source.x\"}");
            try
            {
                Assert.Equal("{\"scopeName\":\"source.x\"}", TextMateJson.ReadAndValidate(path, "p", "grammar"));
            }
            finally
            {
                File.Delete(path);
            }
        }

        [Fact]
        public void AddGrammarFromFile_NonJsonExtension_ThrowsInvalidOperationException()
        {
            // The native file loader only ever parsed '*.json'; routing through the JSON entry
            // point must not quietly widen what this API accepts.
            var path = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName() + ".tmLanguage");
            File.WriteAllText(path, "{\"scopeName\":\"source.x\"}");
            try
            {
                using var registry = new Registry();
                Assert.Throws<InvalidOperationException>(() => registry.AddGrammarFromFile(path));
            }
            finally
            {
                File.Delete(path);
            }
        }

        [Fact]
        public void Registry_AddGrammarFromJson_RejectsMalformedJsonBeforeTheNativeCall()
        {
            using var registry = new Registry();
            Assert.Throws<InvalidOperationException>(() => registry.AddGrammarFromJson("{\"scopeName\":"));
        }

        [Fact]
        public void Theme_LoadFromJson_RejectsMalformedJsonBeforeTheNativeCall()
        {
            Assert.Throws<InvalidOperationException>(() => Theme.LoadFromJson("not json"));
        }

        [Fact]
        public void AddGrammarFromFile_MissingFile_ThrowsInvalidOperationException()
        {
            using var registry = new Registry();
            Assert.Throws<InvalidOperationException>(
                () => registry.AddGrammarFromFile(Path.Combine(Path.GetTempPath(), "no-such-grammar.json")));
        }
    }
}
