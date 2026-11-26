cmake_minimum_required(VERSION 3.25)
project(ax-sdk VERSION 1.36.2 LANGUAGES CXX)

option(AX_SDK_ENABLE_WARNINGS "Enable compiler warnings for ax-sdk targets" OFF)
option(AX_SDK_WERROR "Treat compiler warnings as errors when AX_SDK_ENABLE_WARNINGS is ON" OFF)
option(AX_SDK_STAGE_SOURCE_TREE "Install ax-sdk source tree into <prefix>/share/ax-sdk/src" OFF)

set(_ax_sdk_explicit_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/src/config.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/memory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/persist.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/topology.cpp"
)

set(AX_SDK_SOURCES "")
foreach(_src IN LISTS _ax_sdk_explicit_sources)
  if(EXISTS "${_src}")
    list(APPEND AX_SDK_SOURCES "${_src}")
  endif()
endforeach()

file(GLOB_RECURSE _ax_sdk_discovered CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER _ax_sdk_discovered EXCLUDE REGEX ".*/main\\.cpp$")
foreach(_src IN LISTS _ax_sdk_discovered)
  list(FIND AX_SDK_SOURCES "${_src}" _found_idx)
  if(_found_idx EQUAL -1)
    list(APPEND AX_SDK_SOURCES "${_src}")
  endif()
endforeach()

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
  target_link_libraries(ax_core ${AX_VISIBILITY} Threads::Threads ${CMAKE_DL_LIBS} rt)
elseif(WIN32)
  target_compile_definitions(ax_core ${AX_VISIBILITY} AX_PLATFORM_WINDOWS=1)
  target_compile_definitions(ax_core ${AX_PRIVATE_VISIBILITY} NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(ax_core ${AX_VISIBILITY} winmm avrt psapi)
endif()

if(AX_SDK_SOURCES)
  set(_ax_clang_toolchain_helper "${CMAKE_CURRENT_SOURCE_DIR}/ax_toolchain/piccolo-clang-x86_64.cmake")
  if(EXISTS "${_ax_clang_toolchain_helper}")
    include("${_ax_clang_toolchain_helper}")
  endif()

  if(AX_SDK_ENABLE_WARNINGS)
    if(MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
      target_compile_options(ax_core PRIVATE /W4)
      if(AX_SDK_WERROR)
        target_compile_options(ax_core PRIVATE /WX)
      endif()
    else()
      target_compile_options(ax_core PRIVATE -Wall -Wextra -Wpedantic)
      if(AX_SDK_WERROR)
        target_compile_options(ax_core PRIVATE -Werror)
      endif()
    endif()
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

set(_ax_sdk_config_in "${CMAKE_CURRENT_SOURCE_DIR}/ax_toolchain/ax-sdkConfig.cmake.in")
if(EXISTS "${_ax_sdk_config_in}")
  configure_package_config_file(
    "${_ax_sdk_config_in}"
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
endif()

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
