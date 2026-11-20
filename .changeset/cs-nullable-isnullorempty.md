---
textmatelib: patch
---

[Fixed] Compile the C# bindings without CS8604 on runtimes whose `string.IsNullOrEmpty` lacks the `NotNullWhen` annotation, such as Unity, where warnings-as-errors builds failed
