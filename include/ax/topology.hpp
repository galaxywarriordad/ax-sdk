#pragma once

#include <cstddef>
#include <cstdint>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace ax::topology {

inline constexpr std::uint32_t kAudioCoreId = 0;
inline constexpr std::uint32_t kVideoScanoutCoreId = 1;
inline constexpr std::uint32_t kHousekeepingCoreId = 5;
inline constexpr std::uint64_t kHousekeepingCoreMask = (1ULL << kHousekeepingCoreId);

inline bool pin_thread_to_core(std::uint32_t core_id) noexcept {
#if defined(__linux__)
	if (core_id >= static_cast<std::uint32_t>(CPU_SETSIZE)) {
		return false;
	}
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(static_cast<int>(core_id), &cpuset);
	if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0) {
		return true;
	}
	return sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0;
#else
	(void)core_id;
	return false;
#endif
}

inline bool pin_to_housekeeping_core5() noexcept {
	return pin_thread_to_core(kHousekeepingCoreId);
}

} // namespace ax::topology
