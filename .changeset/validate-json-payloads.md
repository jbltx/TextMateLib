---
textmatelib: patch
---

[Fixed] Validate grammar and theme JSON in the C# bindings before it crosses the P/Invoke boundary, load files by passing the validated content to the native loader instead of a path it would re-read, and route file-load failures (unresolvable, missing, or unreadable paths) through the documented `InvalidOperationException` instead of leaking `IOException`/`UnauthorizedAccessException`
