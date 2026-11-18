# Shared Clang 23.1+ ThinLTO + 3-Phase PGO Toolchain for Piccolo [AX] Emulators
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_LINKER lld)

set(AX_ARCH_FLAGS "-march=skylake -mavx2 -mfma -mbmi -mbmi2 -O3 -flto=thin")
set(AX_PGO_STAGE "0" CACHE STRING "PGO Stage: 0=Baseline, 1=Generate, 2=Use")
set(AX_PGO_PROFILE "/userdata/pgo/ax.profdata" CACHE FILEPATH "Merged llvm-profdata path")

if(AX_PGO_STAGE STREQUAL "1")
    set(AX_PGO_FLAGS "-fprofile-instr-generate=/dev/shm/eden_cache/default_%m.profraw")
elseif(AX_PGO_STAGE STREQUAL "2")
    set(AX_PGO_FLAGS "-fprofile-instr-use=${AX_PGO_PROFILE} -Wno-profile-instr-out-of-date")
else()
    set(AX_PGO_FLAGS "")
endif()

set(CMAKE_C_FLAGS_INIT "${AX_ARCH_FLAGS} ${AX_PGO_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${AX_ARCH_FLAGS} ${AX_PGO_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld -flto=thin ${AX_PGO_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld -flto=thin ${AX_PGO_FLAGS}")
