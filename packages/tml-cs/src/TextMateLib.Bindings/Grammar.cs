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

        int[]? m_CodepointToCharIndex;

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

                // The native library returns Unicode codepoint indices.
                // C# strings use UTF-16 where codepoints above U+FFFF occupy 2 chars
                // (surrogate pairs), so we need to adjust for those.
                // For strings without astral plane characters, codepoint == char index.
                int surrogatePairCount = 0;
                for (int ci = 0; ci < text.Length; ci++)
                {
                    if (char.IsHighSurrogate(text[ci]))
                        surrogatePairCount++;
                }

                // Only build the mapping if there are surrogate pairs
                if (surrogatePairCount > 0)
                {
                    int cpCount = text.Length - surrogatePairCount;
                    int requiredSize = cpCount + 1;
                    if (m_CodepointToCharIndex == null || m_CodepointToCharIndex.Length < requiredSize)
                        m_CodepointToCharIndex = new int[requiredSize];

                    int cpIdx = 0;
                    for (int ci = 0; ci < text.Length; cpIdx++)
                    {
                        m_CodepointToCharIndex[cpIdx] = ci;
                        ci += char.IsHighSurrogate(text[ci]) ? 2 : 1;
                    }
                    m_CodepointToCharIndex[cpIdx] = text.Length; // end sentinel
                }

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

                    int startCharIndex, endCharIndex;
                    if (surrogatePairCount > 0)
                    {
                        int maxIdx = text.Length - surrogatePairCount;
                        startCharIndex = m_CodepointToCharIndex![Math.Min(nativeToken.StartIndex, maxIdx)];
                        endCharIndex = m_CodepointToCharIndex[Math.Min(nativeToken.EndIndex, maxIdx)];
                    }
                    else
                    {
                        // No surrogate pairs: codepoint index == char index
                        startCharIndex = nativeToken.StartIndex;
                        endCharIndex = nativeToken.EndIndex;
                    }

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
