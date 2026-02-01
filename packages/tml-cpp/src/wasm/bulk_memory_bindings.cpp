#ifdef WASM_2023_FEATURES

#include "wasm/bulk_memory_bindings.h"
#include <algorithm>
#include <cstring>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/heap.h>
#endif

// Thread-safe shared memory pool
static thread_local std::vector<std::pair<void*, size_t>> sharedBuffers;
static thread_local size_t totalSharedMemory = 0;
static const size_t MAX_SHARED_MEMORY = 10 * 1024 * 1024; // 10MB limit

size_t BulkMemoryHelper::concatenateStringsBulk(
    const std::vector<std::string>& strings) {
    // Calculate total size needed
    size_t totalSize = 0;
    for (const auto& str : strings) {
        totalSize += str.length();
    }

    if (totalSize == 0) {
        return 0;
    }

    // Allocate buffer
    char* buffer = new char[totalSize];
    size_t offset = 0;

    // Copy all strings into buffer using bulk operations
    for (const auto& str : strings) {
        std::memcpy(buffer + offset, str.data(), str.length());
        offset += str.length();
    }

#ifdef __EMSCRIPTEN__
    // Return pointer as offset from WASM heap base
    return reinterpret_cast<size_t>(buffer);
#else
    return reinterpret_cast<size_t>(buffer);
#endif
}

void BulkMemoryHelper::moveMemoryFast(void* dst, const void* src, size_t size) {
    // memmove handles overlapping regions correctly
    std::memmove(dst, src, size);
}

void* BulkMemoryHelper::getSharedBuffer(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    // Check memory limit
    if (totalSharedMemory + size > MAX_SHARED_MEMORY) {
        return nullptr;
    }

    // Allocate new buffer
    void* buffer = malloc(size);
    if (buffer) {
        sharedBuffers.push_back({buffer, size});
        totalSharedMemory += size;
    }

    return buffer;
}

void BulkMemoryHelper::releaseSharedBuffer(void* ptr) {
    if (!ptr) {
        return;
    }

    // Find and remove buffer
    auto it = std::find_if(
        sharedBuffers.begin(),
        sharedBuffers.end(),
        [ptr](const std::pair<void*, size_t>& p) { return p.first == ptr; });

    if (it != sharedBuffers.end()) {
        totalSharedMemory -= it->second;
        free(ptr);
        sharedBuffers.erase(it);
    }
}

#ifdef __EMSCRIPTEN__

// Helper to create TypedArray from WASM memory
emscripten::val createTypedArray(const void* ptr, size_t count, const char* type) {
    emscripten::val ta = emscripten::val::global(type);
    emscripten::val memory = emscripten::val::global("wasmMemory")["buffer"];

    return ta.new_(memory, reinterpret_cast<size_t>(ptr), count);
}

// Emscripten bindings for bulk memory operations
EMSCRIPTEN_BINDINGS(bulk_memory) {
    emscripten::class_<BulkMemoryHelper>("BulkMemoryHelper")
        .class_function("concatenateStringsBulk",
                       &BulkMemoryHelper::concatenateStringsBulk)
        .class_function("getSharedBuffer",
                       &BulkMemoryHelper::getSharedBuffer)
        .class_function("releaseSharedBuffer",
                       &BulkMemoryHelper::releaseSharedBuffer)
        .class_function("moveMemoryFast",
                       &BulkMemoryHelper::moveMemoryFast);
};

#endif // __EMSCRIPTEN__

#endif // WASM_2023_FEATURES
