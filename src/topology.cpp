#include "ax/topology.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#endif

namespace ax::topology {
namespace {

std::once_flag g_probe_once_flag;
SiliconProfile g_cached_profile;

void PopulateCpuInfo(SiliconProfile& profile) noexcept {
  try {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
      return;
    }
    std::string line;
    while (std::getline(cpuinfo, line)) {
      if (profile.vendor.empty() && line.rfind("vendor_id", 0) == 0) {
        const auto pos = line.find(':');
        if (pos != std::string::npos) {
          profile.vendor = line.substr(pos + 2);
        }
      } else if (profile.model_name.empty() && line.rfind("model name", 0) == 0) {
        const auto pos = line.find(':');
        if (pos != std::string::npos) {
          profile.model_name = line.substr(pos + 2);
        }
      }
    }
  } catch (...) {
  }
}

SiliconProfile BuildDefaultProfile() noexcept {
  SiliconProfile p{};
  const unsigned int hw_threads = std::max(1u, std::thread::hardware_concurrency());
  p.logical_threads = static_cast<std::uint32_t>(hw_threads);
  p.physical_cores = p.logical_threads;
  p.has_smt = false;
  p.is_hybrid = false;

  if (p.physical_cores >= 6) {
    p.audio_mask = (1ULL << kAudioCoreId);
    p.vulkan_mask = (1ULL << kVideoScanoutCoreId);
    p.jit_mask = (1ULL << 2) | (1ULL << 3) | (1ULL << 4);
    p.housekeeping_mask = kHousekeepingCoreMask;
  } else if (p.physical_cores >= 4) {
    p.audio_mask = (1ULL << 0);
    p.vulkan_mask = (1ULL << 1);
    p.jit_mask = (1ULL << 2);
    p.housekeeping_mask = (1ULL << 3);
  } else {
    const std::uint64_t all_mask = (p.physical_cores >= 64)
        ? ~0ULL
        : ((1ULL << p.physical_cores) - 1ULL);
    p.audio_mask = all_mask;
    p.vulkan_mask = all_mask;
    p.jit_mask = all_mask;
    p.housekeeping_mask = all_mask;
  }

  PopulateCpuInfo(p);
  return p;
}

std::uint64_t MaskForDomain(const SiliconProfile& p, Domain domain) noexcept {
  switch (domain) {
    case Domain::Audio:
      return p.audio_mask;
    case Domain::Vulkan:
      return p.vulkan_mask;
    case Domain::Jit:
      return p.jit_mask;
    case Domain::Housekeeping:
      return p.housekeeping_mask;
  }
  return 0;
}

} // namespace

SiliconProfile ProbeHostSiliconOnce() noexcept {
  std::call_once(g_probe_once_flag, []() {
    g_cached_profile = BuildDefaultProfile();
  });
  return g_cached_profile;
}

SiliconProfile DetectHostSilicon() noexcept {
  return ProbeHostSiliconOnce();
}

bool PinCurrentThread(Domain domain, int realtime_priority) noexcept {
  const SiliconProfile profile = ProbeHostSiliconOnce();
  const std::uint64_t mask = MaskForDomain(profile, domain);
  if (mask == 0) {
    return false;
  }

#if defined(__linux__)
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  bool any_set = false;
  for (int cpu = 0; cpu < CPU_SETSIZE && cpu < 64; ++cpu) {
    if ((mask & (1ULL << cpu)) != 0ULL) {
      CPU_SET(cpu, &cpuset);
      any_set = true;
    }
  }
  if (!any_set) {
    return false;
  }

  bool ok = (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0);
  if (!ok) {
    ok = (sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0);
  }

  if (realtime_priority > 0) {
    sched_param sp{};
    sp.sched_priority = realtime_priority;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0) {
      ok = false;
    }
  }
  return ok;
#else
  (void)realtime_priority;
  return false;
#endif
}

} // namespace ax::topology
