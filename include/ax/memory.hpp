#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace ax::memory {

enum class PageTier : std::uint8_t {
    Standard4K = 0,
    Huge2M = 1,
    Huge1G = 2
};

struct ArenaHandle {
    void* base = nullptr;
    std::size_t size_bytes = 0;
    PageTier active_tier = PageTier::Standard4K;
};

[[nodiscard]] ArenaHandle AllocateFastmemArena(std::size_t size_bytes, bool prefer_1gb = true) noexcept;
void FreeFastmemArena(ArenaHandle& arena) noexcept;

[[nodiscard]] inline std::uint64_t ExtractPageIndex(std::uint64_t guest_addr,
                                                    std::uint8_t shift,
                                                    std::uint8_t width) noexcept {
#if defined(__BMI__)
    return _bextr_u64(guest_addr, shift, width);
#else
    return (guest_addr >> shift) & ((1ULL << width) - 1ULL);
#endif
}

inline void StreamPrefetchNTA(const void* addr) noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_NTA);
#else
    (void)addr;
#endif
}

} // namespace ax::memory
