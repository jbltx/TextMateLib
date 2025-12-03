/**
 * WASM Module Loader for TextMateLib
 * Handles loading and initializing the Emscripten-compiled WASM module
 */

import type { TextMateWasmModule, WasmLoaderOptions } from './types';
import { URL } from 'url';
declare const document: any;
declare const window: any;

let moduleInstance: any = null;
let initPromise: Promise<any> | null = null;

/**
 * Get the WASM files path based on the environment
 */
function getWasmPath(customPath?: string): string {
  if (customPath) {
    return customPath.endsWith('/') ? customPath : customPath + '/';
  }

  // In Node.js, resolve relative to the package directory
  if (typeof window === 'undefined') {
    return new URL('../wasm/', import.meta.url).pathname;
  }

  // In browser, use relative path from current location
  return './wasm/';
}

/**
 * Load the WASM module from the Emscripten-compiled JavaScript wrapper
 * This function handles both browser and Node.js environments
 *
 * @param options - Configuration options for loading
 * @returns Promise that resolves to the initialized WASM module
 */
export async function loadTextMateModule(
  options: WasmLoaderOptions = {}
): Promise<TextMateWasmModule> {
  if (moduleInstance) {
    return moduleInstance;
  }

  if (initPromise) {
    return initPromise;
  }

  initPromise = (async () => {
    const wasmPath = getWasmPath(options.wasmPath);
    const scriptPath = `${wasmPath}tml-standard.js`;

    if (options.debug) {
      console.log(`[TextMateLib] Loading WASM from: ${scriptPath}`);
    }

    // Load the Emscripten module
    const module = await loadEmscriptenModule(scriptPath);

    if (options.debug) {
      console.log('[TextMateLib] WASM module loaded successfully');
    }

    moduleInstance = {
      Registry: module.Registry,
      Grammar: module.Grammar,
      _module: module,
    };

    return moduleInstance;
  })();

  return initPromise;
}

/**
 * Load the Emscripten module in both browser and Node.js
 */
async function loadEmscriptenModule(scriptPath: string): Promise<any> {
  return new Promise((resolve, reject) => {
    // Browser environment
    if (typeof window !== 'undefined') {
      return loadInBrowser(scriptPath, resolve, reject);
    }

    // Node.js environment
    return loadInNode(scriptPath, resolve, reject);
  });
}

/**
 * Load WASM module in browser environment
 */
function loadInBrowser(
  scriptPath: string,
  resolve: (module: any) => void,
  reject: (error: Error) => void
): void {
  try {
    const script = document.createElement('script');
    script.src = scriptPath;
    script.async = true;

    script.onload = () => {
      // The Emscripten module is available as a global factory
      // The EXPORT_NAME in CMakeLists.txt is 'createTextMateModule'
      if (typeof (window as any).createTextMateModule === 'function') {
        (window as any)
          .createTextMateModule()
          .then((module: any) => {
            resolve(module);
          })
          .catch((err: Error) => {
            reject(new Error(`Failed to initialize TextMate module: ${err.message}`));
          });
      } else {
        reject(new Error('TextMate WASM module not found after loading script'));
      }
    };

    script.onerror = () => {
      reject(new Error(`Failed to load WASM script: ${scriptPath}`));
    };

    document.head.appendChild(script);
  } catch (err) {
    reject(new Error(`Failed to load WASM in browser: ${err}`));
  }
}

/**
 * Load WASM module in Node.js environment
 */
async function loadInNode(
  scriptPath: string,
  resolve: (module: any) => void,
  reject: (error: Error) => void
): Promise<void> {
  try {
    // Convert file path to file:// URL if needed
    let moduleUrl = scriptPath;
    if (!moduleUrl.startsWith('file://') && !moduleUrl.startsWith('http')) {
      moduleUrl = `file://${moduleUrl}`;
    }

    // Dynamic import the WASM module
    const moduleFactory = await import(moduleUrl);

    // The exported name is 'createTextMateModule'
    if (typeof moduleFactory.default === 'function') {
      const module = await moduleFactory.default();
      resolve(module);
    } else if (typeof moduleFactory.createTextMateModule === 'function') {
      const module = await moduleFactory.createTextMateModule();
      resolve(module);
    } else {
      reject(new Error('TextMate WASM module factory not found in exports'));
    }
  } catch (err) {
    reject(new Error(`Failed to load WASM in Node.js: ${err}`));
  }
}

/**
 * Initialize the WASM module (alias for loadTextMateModule)
 * Useful for ensuring the module is loaded before using APIs
 */
export async function initializeModule(options?: WasmLoaderOptions): Promise<TextMateWasmModule> {
  return loadTextMateModule(options);
}

/**
 * Get the already-loaded module instance
 * Returns null if module hasn't been loaded yet
 */
export function getTextMateModule(): TextMateWasmModule | null {
  return moduleInstance;
}

/**
 * Reset the module instance (useful for testing)
 */
export function resetModule(): void {
  moduleInstance = null;
  initPromise = null;
}
