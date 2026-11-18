#include "ax/memory.hpp"

#ifdef __linux__
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
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace ax::memory {

ArenaHandle AllocateFastmemArena(std::size_t size_bytes, bool prefer_1gb) noexcept {
    ArenaHandle handle{};
    if (size_bytes == 0) {
        return handle;
    }

#ifdef __linux__
    constexpr int kProt = PROT_READ | PROT_WRITE;
    constexpr int kBaseFlags = MAP_PRIVATE | MAP_ANONYMOUS;

    if (prefer_1gb) {
        void* ptr = ::mmap(nullptr, size_bytes, kProt, kBaseFlags | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0);
        if (ptr != MAP_FAILED) {
            ::mlock(ptr, size_bytes);
            return ArenaHandle{ptr, size_bytes, PageTier::Huge1G};
        }
    }

    void* ptr_2m = ::mmap(nullptr, size_bytes, kProt, kBaseFlags | MAP_HUGETLB | MAP_HUGE_2MB, -1, 0);
    if (ptr_2m != MAP_FAILED) {
        ::mlock(ptr_2m, size_bytes);
        return ArenaHandle{ptr_2m, size_bytes, PageTier::Huge2M};
    }

    void* ptr_4k = ::mmap(nullptr, size_bytes, kProt, kBaseFlags, -1, 0);
    if (ptr_4k != MAP_FAILED) {
        ::mlock(ptr_4k, size_bytes);
        return ArenaHandle{ptr_4k, size_bytes, PageTier::Standard4K};
    }
#elif defined(_WIN32)
    (void)prefer_1gb;
    void* ptr_lp = ::VirtualAlloc(nullptr, size_bytes, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
    if (ptr_lp != nullptr) {
        return ArenaHandle{ptr_lp, size_bytes, PageTier::Huge2M};
    }
    void* ptr_4k = ::VirtualAlloc(nullptr, size_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (ptr_4k != nullptr) {
        return ArenaHandle{ptr_4k, size_bytes, PageTier::Standard4K};
    }
#else
    (void)prefer_1gb;
#endif
    return handle;
}

void FreeFastmemArena(ArenaHandle& arena) noexcept {
    if (arena.base == nullptr) {
        return;
    }
#ifdef __linux__
    ::munlock(arena.base, arena.size_bytes);
    ::munmap(arena.base, arena.size_bytes);
#elif defined(_WIN32)
    ::VirtualFree(arena.base, 0, MEM_RELEASE);
#endif
    arena = ArenaHandle{};
}

} // namespace ax::memory
