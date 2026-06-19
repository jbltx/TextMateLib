---
textmatelib: patch
---

[Fixed] Harden the C FFI producers with RAII so a mid-construction allocation failure frees partial allocations instead of leaking, null-check batch and color-map result pointers before dereferencing, and pool the encoded-token copy buffer in the C# bindings to remove per-line GC pressure
