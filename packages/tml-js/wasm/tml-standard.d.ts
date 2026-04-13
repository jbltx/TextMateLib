import type { WasmModule } from '../src/types';

declare function createModule(): Promise<WasmModule>;
export default createModule;
