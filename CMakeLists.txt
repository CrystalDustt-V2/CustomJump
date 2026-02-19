cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(CustomJump VERSION 1.0.0)

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})

# [Geode v5.0.0-beta.3 Migration]
# MSVC does not expose __builtin_memset by default, but current beta.3 event
# templates instantiate std23::function paths that reference it.
if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE __builtin_memset=memset)
endif()

# fmt emits a known deprecated-locale warning on newer Clang/libc++.
# Keep CI output clean without muting warnings for this mod's own code.
if (TARGET fmt)
    target_compile_options(fmt PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-Wno-deprecated-declarations>
    )
endif()
