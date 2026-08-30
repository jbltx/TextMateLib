---
textmatelib: patch
---

[Fixed] Validate grammar and theme JSON in the C# bindings before it crosses the P/Invoke boundary, and route file-load failures (unresolvable, missing, or unreadable paths) through the documented `InvalidOperationException` instead of leaking `IOException`/`UnauthorizedAccessException`
