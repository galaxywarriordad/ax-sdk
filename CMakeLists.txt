cmake_minimum_required(VERSION 3.25)
project(ax-sdk VERSION 1.35.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE AX_SDK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ax_core STATIC ${AX_SDK_SOURCES})
add_library(ax::core ALIAS ax_core)

target_include_directories(ax_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ax_core PUBLIC pthread dl)
endif()
