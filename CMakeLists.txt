cmake_minimum_required(VERSION 3.25)
project(ax-sdk VERSION 1.35.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE AX_SDK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(AX_SDK_SOURCES)
    add_library(ax_core STATIC ${AX_SDK_SOURCES})
    target_include_directories(ax_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(ax_core PUBLIC pthread dl)
    elseif(WIN32)
        target_compile_definitions(ax_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
        target_link_libraries(ax_core PUBLIC winmm avrt psapi)
    endif()
else()
    add_library(ax_core INTERFACE)
    target_include_directories(ax_core INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(ax_core INTERFACE pthread dl)
    elseif(WIN32)
        target_compile_definitions(ax_core INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
        target_link_libraries(ax_core INTERFACE winmm avrt psapi)
    endif()
endif()

add_library(ax::core ALIAS ax_core)
add_library(ax::sdk ALIAS ax_core)
