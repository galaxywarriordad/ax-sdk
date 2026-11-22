#include "ax/memory.hpp"

#include <cstddef>
#include <cstdint>

#if defined(__linux__)
#include <linux/mman.h>
#include <sys/mman.h>
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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ax::memory {

ArenaHandle AllocateFastmemArena(std::size_t size_bytes, bool prefer_1gb) noexcept {
    ArenaHandle handle{};
    if (size_bytes == 0) {
        return handle;
    }

#if defined(__linux__)
    constexpr std::size_t kOneGiB = 1ULL << 30;
    constexpr std::size_t kTwoMiB = 2ULL << 20;
    const int base_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;

    if (prefer_1gb && (size_bytes % kOneGiB == 0)) {
        void* ptr = ::mmap(nullptr, size_bytes, PROT_READ | PROT_WRITE,
                           base_flags | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0);
        if (ptr != MAP_FAILED) {
            handle.base = ptr;
            handle.size_bytes = size_bytes;
            handle.active_tier = PageTier::Huge1G;
            return handle;
        }
    }

    if (size_bytes % kTwoMiB == 0) {
        void* ptr = ::mmap(nullptr, size_bytes, PROT_READ | PROT_WRITE,
                           base_flags | MAP_HUGETLB | MAP_HUGE_2MB, -1, 0);
        if (ptr != MAP_FAILED) {
            handle.base = ptr;
            handle.size_bytes = size_bytes;
            handle.active_tier = PageTier::Huge2M;
            return handle;
        }
    }

    void* fallback = ::mmap(nullptr, size_bytes, PROT_READ | PROT_WRITE, base_flags, -1, 0);
    if (fallback != MAP_FAILED) {
        handle.base = fallback;
        handle.size_bytes = size_bytes;
        handle.active_tier = PageTier::Standard4K;
    }
    return handle;

#elif defined(_WIN32)
    const SIZE_T min_large = ::GetLargePageMinimum();
    if (min_large > 0 && (size_bytes % min_large == 0)) {
        void* ptr = ::VirtualAlloc(nullptr, size_bytes,
                                   MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                                   PAGE_READWRITE);
        if (ptr != nullptr) {
            handle.base = ptr;
            handle.size_bytes = size_bytes;
            handle.active_tier = (min_large >= (1ULL << 30)) ? PageTier::Huge1G : PageTier::Huge2M;
            return handle;
        }
    }

    void* fallback = ::VirtualAlloc(nullptr, size_bytes,
                                    MEM_RESERVE | MEM_COMMIT,
                                    PAGE_READWRITE);
    if (fallback != nullptr) {
        handle.base = fallback;
        handle.size_bytes = size_bytes;
        handle.active_tier = PageTier::Standard4K;
    }
    return handle;

#else
    (void)prefer_1gb;
    return handle;
#endif
}

void FreeFastmemArena(ArenaHandle& arena) noexcept {
    if (arena.base == nullptr) {
        arena.size_bytes = 0;
        arena.active_tier = PageTier::Standard4K;
        return;
    }

#if defined(__linux__)
    if (arena.size_bytes > 0) {
        (void)::munmap(arena.base, arena.size_bytes);
    }
#elif defined(_WIN32)
    (void)::VirtualFree(arena.base, 0, MEM_RELEASE);
#endif

    arena.base = nullptr;
    arena.size_bytes = 0;
    arena.active_tier = PageTier::Standard4K;
}

} // namespace ax::memory
