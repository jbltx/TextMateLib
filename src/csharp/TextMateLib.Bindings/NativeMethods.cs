using System;
using System.Runtime.InteropServices;

namespace TextMateLib.Bindings
{
    /// <summary>
    /// Native method declarations for TextMateLib C API
    /// </summary>
    internal static class NativeMethods
    {
        private const string LibraryName = "tml";

        // ============================================================================
        // Opaque handle types
        // ============================================================================
        
        internal struct TextMateRegistry { public IntPtr Handle; }
        internal struct TextMateGrammar { public IntPtr Handle; }
        internal struct TextMateStateStack { public IntPtr Handle; }
        internal struct TextMateOnigLib { public IntPtr Handle; }
        internal struct TextMateTheme { public IntPtr Handle; }

        // ============================================================================
        // Token structures
        // ============================================================================

        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateToken
        {
            public int StartIndex;
            public int EndIndex;
            public int ScopeDepth;
            public IntPtr Scopes; // char**
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateTokenizeResult
        {
            public IntPtr Tokens; // TextMateToken*
            public int TokenCount;
            public TextMateStateStack RuleStack;
            public int StoppedEarly;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateTokenizeResult2
        {
            public IntPtr Tokens; // uint32_t*
            public int TokenCount;
            public TextMateStateStack RuleStack;
            public int StoppedEarly;
        }

        // ============================================================================
        // Theme API
        // ============================================================================

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern TextMateTheme textmate_theme_load_from_file(
            string themePath);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern TextMateTheme textmate_theme_load_from_json(
            string jsonContent);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern uint textmate_theme_get_foreground(
            TextMateTheme theme,
            string scopePath,
            uint defaultColor);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern uint textmate_theme_get_background(
            TextMateTheme theme,
            string scopePath,
            uint defaultColor);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern int textmate_theme_get_font_style(
            TextMateTheme theme,
            string scopePath,
            int defaultStyle);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint textmate_theme_get_default_foreground(TextMateTheme theme);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint textmate_theme_get_default_background(TextMateTheme theme);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_theme_dispose(TextMateTheme theme);

        // ============================================================================
        // Registry and Grammar API
        // ============================================================================

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern TextMateOnigLib textmate_oniglib_create();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_registry_dispose(TextMateRegistry registry);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern int textmate_registry_add_grammar_from_file(
            TextMateRegistry registry,
            string grammarPath);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern int textmate_registry_add_grammar_from_json(
            TextMateRegistry registry,
            string jsonContent);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern TextMateGrammar textmate_registry_load_grammar(
            TextMateRegistry registry,
            string scopeName);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern TextMateStateStack textmate_get_initial_state();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_tokenize_line(
            TextMateGrammar grammar,
            string lineText,
            TextMateStateStack prevState);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_tokenize_line2(
            TextMateGrammar grammar,
            string lineText,
            TextMateStateStack prevState);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_free_tokenize_result(IntPtr result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_free_tokenize_result2(IntPtr result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_grammar_get_scope_name(TextMateGrammar grammar);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_grammar_dispose(TextMateGrammar grammar);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_oniglib_dispose(TextMateOnigLib onigLib);
    }
}
