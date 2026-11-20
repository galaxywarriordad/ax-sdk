cmake_minimum_required(VERSION 3.25)
project(ax-sdk VERSION 1.36.0 LANGUAGES CXX)

option(AX_SDK_ENABLE_WARNINGS "Enable strict compiler warnings for ax-sdk targets" OFF)

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

target_compile_features(ax_core ${AX_VISIBILITY} cxx_std_20)

target_include_directories(ax_core ${AX_VISIBILITY}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(ax_core ${AX_VISIBILITY} Threads::Threads ${CMAKE_DL_LIBS})
elseif(WIN32)
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

add_library(ax::core ALIAS ax_core)
add_library(ax::sdk ALIAS ax_core)

include(GNUInstallDirs)

install(TARGETS ax_core
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/ax"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
