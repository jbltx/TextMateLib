using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace TextMateLib.Bindings
{
    /// <summary>
    /// Represents a TextMate grammar for tokenizing source code
    /// </summary>
    public class Grammar : IDisposable
    {
        IntPtr m_Handle;

        bool m_Disposed;

        internal Grammar(IntPtr handle)
        {
            m_Handle = handle;
        }

        /// <summary>
        /// Gets the scope name of the grammar (e.g., "source.js")
        /// </summary>
        public string ScopeName
        {
            get
            {
                ThrowIfDisposed();
                IntPtr ptr = NativeMethods.textmate_grammar_get_scope_name(m_Handle);
                return Marshal.PtrToStringAnsi(ptr) ?? string.Empty;
            }
        }

        /// <summary>
        /// Tokenizes a line of text
        /// </summary>
        /// <param name="lineText">The line to tokenize</param>
        /// <param name="prevState">Previous state stack (use IntPtr.Zero for initial state)</param>
        /// <returns>Tokenization result with tokens and state</returns>
        public TokenizeResult TokenizeLine(string lineText, IntPtr prevState)
        {
            ThrowIfDisposed();

            if (prevState == IntPtr.Zero)
            {
                prevState = NativeMethods.textmate_get_initial_state();
            }

            var text = lineText ?? string.Empty;

            // Manually encode to UTF-8 with null terminator.
            // This avoids CharSet.Ansi which uses the system ANSI code page on Windows,
            // corrupting non-ASCII characters.
            var utf8ByteCount = Encoding.UTF8.GetByteCount(text);
            var utf8Bytes = new byte[utf8ByteCount + 1]; // +1 for null terminator
            Encoding.UTF8.GetBytes(text, 0, text.Length, utf8Bytes, 0);

            var resultPtr = NativeMethods.textmate_tokenize_line(m_Handle, utf8Bytes, prevState);

            if (resultPtr == IntPtr.Zero)
                throw new InvalidOperationException("Failed to tokenize line");

            try
            {
                var result = Marshal.PtrToStructure<NativeMethods.TextMateTokenizeResult>(resultPtr);
                var tokens = new List<Token>();

                // Build a mapping from UTF-8 byte offsets (returned by the native library)
                // to UTF-16 char indices (used by C# string operations).
                // For pure ASCII this is an identity mapping, but for multi-byte characters
                // (accented chars, CJK, emojis) the offsets diverge.
                var byteToCharIndex = BuildUtf8ByteToCharIndexMap(text, utf8ByteCount);

                // Convert native tokens to managed tokens
                for (int i = 0; i < result.TokenCount; i++)
                {
                    IntPtr tokenPtr = result.Tokens + i * Marshal.SizeOf<NativeMethods.TextMateToken>();
                    var nativeToken = Marshal.PtrToStructure<NativeMethods.TextMateToken>(tokenPtr);

                    // Extract scopes
                    var scopes = new List<string>();
                    for (int j = 0; j < nativeToken.ScopeDepth; j++)
                    {
                        IntPtr scopePtr = Marshal.ReadIntPtr(nativeToken.Scopes, j * IntPtr.Size);
                        if (scopePtr != IntPtr.Zero)
                        {
                            var scope = Marshal.PtrToStringAnsi(scopePtr);
                            if (!string.IsNullOrEmpty(scope))
                                scopes.Add(scope);
                        }
                    }

                    // Convert UTF-8 byte offsets to UTF-16 char indices
                    var startCharIndex = byteToCharIndex[Math.Min(nativeToken.StartIndex, utf8ByteCount)];
                    var endCharIndex = byteToCharIndex[Math.Min(nativeToken.EndIndex, utf8ByteCount)];

                    tokens.Add(new Token(startCharIndex, endCharIndex, scopes));
                }

                return new TokenizeResult(tokens, result.RuleStack, result.StoppedEarly != 0);
            }
            finally
            {
                NativeMethods.textmate_free_tokenize_result(resultPtr);
            }
        }

        /// <summary>
        /// Tokenizes multiple lines sequentially
        /// </summary>
        /// <param name="lines">Array of lines to tokenize</param>
        /// <returns>Array of tokenization results, one per line</returns>
        public TokenizeResult[] TokenizeLines(string[] lines)
        {
            ThrowIfDisposed();

            if (lines == null || lines.Length == 0)
                return Array.Empty<TokenizeResult>();

            var results = new TokenizeResult[lines.Length];
            IntPtr state = NativeMethods.textmate_get_initial_state();

            for (int i = 0; i < lines.Length; i++)
            {
                results[i] = TokenizeLine(lines[i], state);
                state = results[i].StateStack;
            }

            return results;
        }

        /// <summary>
        /// Builds a lookup table mapping UTF-8 byte offsets to UTF-16 char indices.
        /// </summary>
        static int[] BuildUtf8ByteToCharIndexMap(string text, int utf8ByteCount)
        {
            var map = new int[utf8ByteCount + 1];
            int charIdx = 0;
            int byteIdx = 0;

            while (charIdx < text.Length && byteIdx < utf8ByteCount)
            {
                int codepoint = char.ConvertToUtf32(text, charIdx);
                int utf8Len = codepoint <= 0x7F ? 1
                    : codepoint <= 0x7FF ? 2
                    : codepoint <= 0xFFFF ? 3
                    : 4;
                int charLen = codepoint > 0xFFFF ? 2 : 1; // surrogate pair

                for (int k = 0; k < utf8Len; k++)
                    map[byteIdx + k] = charIdx;

                byteIdx += utf8Len;
                charIdx += charLen;
            }

            // End sentinel: maps one-past-last byte offset to one-past-last char index
            if (byteIdx <= utf8ByteCount)
                map[byteIdx] = charIdx;

            return map;
        }

        internal IntPtr Handle
        {
            get
            {
                ThrowIfDisposed();
                return m_Handle;
            }
        }

        void ThrowIfDisposed()
        {
            if (m_Disposed)
                throw new ObjectDisposedException(nameof(Grammar));
        }

        /// <summary>
        /// Releases the managed handle reference. The underlying native Grammar is owned and disposed by the Registry.
        /// </summary>
        public void Dispose()
        {
            if (!m_Disposed)
            {
                if (m_Handle != IntPtr.Zero)
                {
                    m_Handle = IntPtr.Zero;
                }
                m_Disposed = true;
            }
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Finalizer to ensure resources are released
        /// </summary>
        ~Grammar()
        {
            Dispose();
        }
    }
}
