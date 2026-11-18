set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_LINKER lld)

set(AX_ARCH_FLAGS "-march=skylake -mtune=skylake -mbmi -mbmi2 -mavx2 -mfma -O3 -DNDEBUG")
set(AX_LTO_FLAGS "-flto=thin -fuse-ld=lld")

# Optional 3-Phase PGO Mode: OFF | INSTRUMENT | USE
set(AX_PGO_MODE "OFF" CACHE STRING "PGO build phase (OFF, INSTRUMENT, USE)")
set(AX_PGO_PROFILE_PATH "${CMAKE_BINARY_DIR}/default.profdata" CACHE PATH "Path to merged profdata")

set(AX_PGO_FLAGS "")
if(AX_PGO_MODE STREQUAL "INSTRUMENT")
    set(AX_PGO_FLAGS "-fprofile-generate=${CMAKE_BINARY_DIR}/pgo-raw")
elseif(AX_PGO_MODE STREQUAL "USE")
    set(AX_PGO_FLAGS "-fprofile-use=${AX_PGO_PROFILE_PATH} -Wno-profile-instr-out-of-date")
endif()

set(CMAKE_C_FLAGS_INIT "${AX_ARCH_FLAGS} ${AX_LTO_FLAGS} ${AX_PGO_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${AX_ARCH_FLAGS} ${AX_LTO_FLAGS} ${AX_PGO_FLAGS} -std=c++20")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${AX_LTO_FLAGS} ${AX_PGO_FLAGS} -Wl,--gc-sections")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${AX_LTO_FLAGS} ${AX_PGO_FLAGS} -Wl,--gc-sections")
