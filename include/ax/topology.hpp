#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace ax::topology {

enum class Domain : std::uint8_t {
	Audio = 0,
	Vulkan = 1,
	Jit = 2,
	Housekeeping = 3,
};

struct SiliconProfile {
	std::uint32_t physical_cores = 0;
	std::uint32_t logical_threads = 0;
	std::uint64_t audio_mask = 0;
	std::uint64_t vulkan_mask = 0;
	std::uint64_t jit_mask = 0;
	std::uint64_t housekeeping_mask = 0;
	bool has_smt = false;
	bool is_hybrid = false;
	std::string vendor;
	std::string model_name;
};

inline constexpr std::uint32_t kAudioCoreId = 0;
inline constexpr std::uint32_t kVideoScanoutCoreId = 1;
inline constexpr std::uint32_t kHousekeepingCoreId = 5;
inline constexpr std::uint64_t kHousekeepingCoreMask = (1ULL << kHousekeepingCoreId);

[[nodiscard]] SiliconProfile ProbeHostSiliconOnce() noexcept;
[[nodiscard]] SiliconProfile DetectHostSilicon() noexcept;
bool PinCurrentThread(Domain domain, int realtime_priority = 0) noexcept;

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
