#include "ax/memory.hpp"
#include <cstdint>
#include <cstring>

#if defined(__linux__)
#include <sys/mman.h>
#include <linux/mman.h>
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif
#ifndef MAP_HUGE_2MB
#define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
#endif
#ifndef MAP_HUGE_1GB
#define MAP_HUGE_1GB (30 << MAP_HUGE_SHIFT)
#endif
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace ax::memory {

void* allocate_fastmem_arena(std::size_t size_bytes, bool prefer_1gb) noexcept {
#if defined(__linux__)
    int base_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;

    if (prefer_1gb && (size_bytes % (1ULL << 30) == 0)) {
        void* ptr = mmap(nullptr, size_bytes, PROT_READ | PROT_WRITE,
                         base_flags | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0);
        if (ptr != MAP_FAILED) {
            return ptr;
        }
    }

    if (size_bytes % (2ULL << 20) == 0) {
        void* ptr = mmap(nullptr, size_bytes, PROT_READ | PROT_WRITE,
                         base_flags | MAP_HUGETLB | MAP_HUGE_2MB, -1, 0);
        if (ptr != MAP_FAILED) {
            return ptr;
        }
    }

    void* fallback = mmap(nullptr, size_bytes, PROT_READ | PROT_WRITE, base_flags, -1, 0);
    return (fallback == MAP_FAILED) ? nullptr : fallback;

#elif defined(_WIN32)
    SIZE_T min_large = GetLargePageMinimum();
    if (min_large > 0 && (size_bytes % min_large == 0)) {
        void* ptr = VirtualAlloc(nullptr, size_bytes,
                                 MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                                 PAGE_READWRITE);
        if (ptr != nullptr) {
            return ptr;
        }
    }
    return VirtualAlloc(nullptr, size_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    (void)prefer_1gb;
    return nullptr;
#endif
}

void free_fastmem_arena(void* ptr, std::size_t size_bytes) noexcept {
    if (!ptr) return;
#if defined(__linux__)
    munmap(ptr, size_bytes);
#elif defined(_WIN32)
    (void)size_bytes;
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    (void)size_bytes;
#endif
}

} // namespace ax::memory
