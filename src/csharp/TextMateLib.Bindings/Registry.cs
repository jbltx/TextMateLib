using System;

namespace TextMateLib.Bindings
{
    /// <summary>
    /// Registry for managing grammars and themes
    /// </summary>
    public class Registry : IDisposable
    {
        private NativeMethods.TextMateRegistry _handle;
        private NativeMethods.TextMateOnigLib _onigLib;
        private bool _disposed;

        /// <summary>
        /// Creates a new grammar registry
        /// </summary>
        public Registry()
        {
            _onigLib = NativeMethods.textmate_oniglib_create();
            if (_onigLib.Handle == IntPtr.Zero)
                throw new InvalidOperationException("Failed to initialize Oniguruma library");

            _handle = NativeMethods.textmate_registry_create(_onigLib);
            if (_handle.Handle == IntPtr.Zero)
            {
                NativeMethods.textmate_oniglib_dispose(_onigLib);
                throw new InvalidOperationException("Failed to create registry");
            }
        }

        /// <summary>
        /// Adds a grammar to the registry from a JSON file
        /// </summary>
        /// <param name="grammarPath">Path to the grammar JSON file</param>
        /// <exception cref="ArgumentNullException">Thrown when grammarPath is null or empty</exception>
        /// <exception cref="InvalidOperationException">Thrown when the grammar fails to load</exception>
        public void AddGrammarFromFile(string grammarPath)
        {
            ThrowIfDisposed();
            
            if (string.IsNullOrEmpty(grammarPath))
                throw new ArgumentNullException(nameof(grammarPath));

            int result = NativeMethods.textmate_registry_add_grammar_from_file(_handle, grammarPath);
            if (result != 0)
                throw new InvalidOperationException($"Failed to add grammar from file: {grammarPath}");
        }

        /// <summary>
        /// Adds a grammar to the registry from a JSON string
        /// </summary>
        /// <param name="jsonContent">JSON content of the grammar</param>
        /// <exception cref="ArgumentNullException">Thrown when jsonContent is null or empty</exception>
        /// <exception cref="InvalidOperationException">Thrown when the grammar fails to load</exception>
        public void AddGrammarFromJson(string jsonContent)
        {
            ThrowIfDisposed();
            
            if (string.IsNullOrEmpty(jsonContent))
                throw new ArgumentNullException(nameof(jsonContent));

            int result = NativeMethods.textmate_registry_add_grammar_from_json(_handle, jsonContent);
            if (result != 0)
                throw new InvalidOperationException("Failed to add grammar from JSON");
        }

        /// <summary>
        /// Loads a grammar by scope name
        /// </summary>
        /// <param name="scopeName">The scope name (e.g., "source.js")</param>
        /// <returns>A Grammar instance for the requested scope</returns>
        /// <exception cref="ArgumentNullException">Thrown when scopeName is null or empty</exception>
        /// <exception cref="InvalidOperationException">Thrown when the grammar fails to load</exception>
        public Grammar LoadGrammar(string scopeName)
        {
            ThrowIfDisposed();
            
            if (string.IsNullOrEmpty(scopeName))
                throw new ArgumentNullException(nameof(scopeName));

            var handle = NativeMethods.textmate_registry_load_grammar(_handle, scopeName);
            if (handle.Handle == IntPtr.Zero)
                throw new InvalidOperationException($"Failed to load grammar for scope: {scopeName}");

            return new Grammar(handle);
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(Registry));
        }

        /// <summary>
        /// Releases the native resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed)
            {
                if (_handle.Handle != IntPtr.Zero)
                {
                    NativeMethods.textmate_registry_dispose(_handle);
                    _handle.Handle = IntPtr.Zero;
                }

                if (_onigLib.Handle != IntPtr.Zero)
                {
                    NativeMethods.textmate_oniglib_dispose(_onigLib);
                    _onigLib.Handle = IntPtr.Zero;
                }
                
                _disposed = true;
            }
            GC.SuppressFinalize(this);
        }

        ~Registry()
        {
            Dispose();
        }
    }
}
