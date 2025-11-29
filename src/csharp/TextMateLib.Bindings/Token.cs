using System;

namespace TextMateLib.Bindings
{
    /// <summary>
    /// Represents a syntax token with position and scope information
    /// </summary>
    public class Token
    {
        /// <summary>
        /// Gets the starting position of the token in the line
        /// </summary>
        public int StartIndex { get; }

        /// <summary>
        /// Gets the ending position of the token in the line
        /// </summary>
        public int EndIndex { get; }

        /// <summary>
        /// Gets the scope stack for this token
        /// </summary>
        public IReadOnlyList<string> Scopes { get; }

        internal Token(int startIndex, int endIndex, IReadOnlyList<string> scopes)
        {
            StartIndex = startIndex;
            EndIndex = endIndex;
            Scopes = scopes;
        }

        /// <summary>
        /// Gets the length of the token
        /// </summary>
        public int Length => EndIndex - StartIndex;

        /// <summary>
        /// Gets the value of the token from the source line
        /// </summary>
        public string GetValue(string line)
        {
            if (string.IsNullOrEmpty(line))
                return string.Empty;

            if (StartIndex >= line.Length)
                return string.Empty;

            int length = Math.Min(EndIndex, line.Length) - StartIndex;
            return line.Substring(StartIndex, length);
        }

        public override string ToString()
        {
            return $"Token [{StartIndex}..{EndIndex}] Scopes: {string.Join(", ", Scopes)}";
        }
    }

    /// <summary>
    /// Represents the result of tokenizing a line
    /// </summary>
    public class TokenizeResult
    {
        /// <summary>
        /// Gets the tokens for the line
        /// </summary>
        public IReadOnlyList<Token> Tokens { get; }

        /// <summary>
        /// Gets the state stack at the end of the line
        /// </summary>
        public IntPtr StateStack { get; }

        /// <summary>
        /// Gets a value indicating whether tokenization stopped early
        /// </summary>
        public bool StoppedEarly { get; }

        internal TokenizeResult(IReadOnlyList<Token> tokens, IntPtr stateStack, bool stoppedEarly)
        {
            Tokens = tokens;
            StateStack = stateStack;
            StoppedEarly = stoppedEarly;
        }
    }
}
