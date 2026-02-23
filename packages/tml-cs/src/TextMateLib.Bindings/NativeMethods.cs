using System;
using System.Runtime.InteropServices;

namespace TextMateLib.Bindings
{
    /// <summary>
    /// Native method declarations for TextMateLib C API
    /// </summary>
    static class NativeMethods
    {
        const string LibraryName = "tml";

        // ============================================================================
        // Opaque handle types
        // ============================================================================

        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateRegistry { public IntPtr Handle; }
        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateGrammar { public IntPtr Handle; }
        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateStateStack { public IntPtr Handle; }
        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateOnigLib { public IntPtr Handle; }
        [StructLayout(LayoutKind.Sequential)]
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
            public IntPtr RuleStack;
            public int StoppedEarly;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct TextMateTokenizeResult2
        {
            public IntPtr Tokens; // uint32_t*
            public int TokenCount;
            public IntPtr RuleStack;
            public int StoppedEarly;
        }

        // ============================================================================
        // Theme API
        // ============================================================================

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_theme_load_from_file(
            string themePath);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_theme_load_from_json(
            string jsonContent);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern uint textmate_theme_get_foreground(
            IntPtr theme,
            string scopePath,
            uint defaultColor);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern uint textmate_theme_get_background(
            IntPtr theme,
            string scopePath,
            uint defaultColor);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern int textmate_theme_get_font_style(
            IntPtr theme,
            string scopePath,
            int defaultStyle);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint textmate_theme_get_default_foreground(IntPtr theme);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern uint textmate_theme_get_default_background(IntPtr theme);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_theme_dispose(IntPtr theme);

        // ============================================================================
        // Registry and Grammar API
        // ============================================================================

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr textmate_oniglib_create();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr textmate_registry_create(IntPtr onigLib);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_registry_dispose(IntPtr registry);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern int textmate_registry_add_grammar_from_file(
            IntPtr registry,
            string grammarPath);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern int textmate_registry_add_grammar_from_json(
            IntPtr registry,
            string jsonContent);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_registry_load_grammar(
            IntPtr registry,
            string scopeName);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr textmate_get_initial_state();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr textmate_tokenize_line(
            IntPtr grammar,
            byte[] lineTextUtf8,
            IntPtr prevState);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr textmate_tokenize_line2(
            IntPtr grammar,
            byte[] lineTextUtf8,
            IntPtr prevState);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_free_tokenize_result(IntPtr result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_free_tokenize_result2(IntPtr result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern IntPtr textmate_grammar_get_scope_name(IntPtr grammar);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void textmate_oniglib_dispose(IntPtr onigLib);
    }
}
