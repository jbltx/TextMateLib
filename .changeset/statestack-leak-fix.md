---
textmatelib: patch
---

Fix an unbounded leak of internal StateStack nodes in the WASM bindings that could exhaust memory and crash on large inputs (`std::bad_alloc` surfacing as a `WebAssembly.Exception`). Tokenization now reclaims these nodes via an opt-in arena/mark-sweep, and scope-token marshalling across the WASM boundary is flattened for faster tokenization with identical output.
