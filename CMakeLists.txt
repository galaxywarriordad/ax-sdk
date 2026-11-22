cmake_minimum_required(VERSION 3.25)
project(ax-sdk VERSION 1.36.1 LANGUAGES CXX)

option(AX_SDK_ENABLE_WARNINGS "Enable strict compiler warnings for ax-sdk targets" OFF)
option(AX_SDK_STAGE_SOURCE_TREE "Install ax-sdk source tree into <prefix>/share/ax-sdk/src" OFF)

file(GLOB_RECURSE AX_SDK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(AX_SDK_SOURCES)
    add_library(ax_core STATIC ${AX_SDK_SOURCES})
    set(AX_VISIBILITY PUBLIC)
    set(AX_PRIVATE_VISIBILITY PRIVATE)
else()
    add_library(ax_core INTERFACE)
    set(AX_VISIBILITY INTERFACE)
    set(AX_PRIVATE_VISIBILITY INTERFACE)
endif()

add_library(ax::core ALIAS ax_core)
add_library(ax::sdk ALIAS ax_core)

target_compile_features(ax_core ${AX_VISIBILITY} cxx_std_20)

target_include_directories(ax_core ${AX_VISIBILITY}
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_compile_definitions(ax_core ${AX_VISIBILITY} AX_PLATFORM_LINUX=1)
    target_link_libraries(ax_core ${AX_VISIBILITY} Threads::Threads${CMAKE_DL_LIBS})
elseif(WIN32)
    target_compile_definitions(ax_core ${AX_VISIBILITY} AX_PLATFORM_WINDOWS=1)
    target_compile_definitions(ax_core ${AX_PRIVATE_VISIBILITY} NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(ax_core ${AX_VISIBILITY} winmm avrt psapi)
endif()

if(AX_SDK_ENABLE_WARNINGS AND AX_SDK_SOURCES)
    if(MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        target_compile_options(ax_core PRIVATE /W4 /WX)
    else()
        target_compile_options(ax_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ax_core
    EXPORT ax-sdkTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/ax"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT ax-sdkTargets
    FILE ax-sdkTargets.cmake
    NAMESPACE ax::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ax-sdk"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ax_toolchain/ax-sdkConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ax-sdkConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ax-sdk"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ax-sdkConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ax-sdkConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ax-sdkConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ax-sdk"
)

if(AX_SDK_STAGE_SOURCE_TREE)
    install(
        DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/config"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/ax-sdk/src"
        OPTIONAL
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/ax-sdk/src"
    )
endif()
```[cite: 19]

---

### 2. `ax-sdk/src/memory.cpp` (Full File — Create in `ax-sdk` Repo)

```cpp
#include "ax/memory.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

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
