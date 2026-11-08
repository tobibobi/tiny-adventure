cmake_minimum_required(VERSION 3.16)

project(tinyadv
    VERSION 0.1.0
    DESCRIPTION "A tiny, data-driven text adventure engine"
    LANGUAGES C)

# --------------------------------------------------------------------------
# Global settings
# --------------------------------------------------------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(TINYADV_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TINYADV_SANITIZE "Build with address and undefined-behaviour sanitizers" OFF)

include(GNUInstallDirs)
include(CTest)

# Shared compiler settings, linked by every target in the project.
add_library(project_options INTERFACE)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(project_options INTERFACE
        -Wall -Wextra -pedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes
        $<$<BOOL:${TINYADV_WARNINGS_AS_ERRORS}>:-Werror>)
    if(TINYADV_SANITIZE)
        target_compile_options(project_options INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(project_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# --------------------------------------------------------------------------
# Modules and applications
# --------------------------------------------------------------------------
add_subdirectory(sources/modules/advengine)
add_subdirectory(sources/apps/tinyadv)

# --------------------------------------------------------------------------
# Documentation (optional, needs Doxygen)
# --------------------------------------------------------------------------
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
    set(DOXYGEN_PROJECT_NUMBER ${PROJECT_VERSION})
    set(DOXYGEN_PROJECT_BRIEF "${PROJECT_DESCRIPTION}")
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/readme.md)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/readme.md
        ${CMAKE_SOURCE_DIR}/docs
        ${CMAKE_SOURCE_DIR}/sources
        COMMENT "Generating API documentation")
else()
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E echo "Doxygen not found - install it to generate docs."
        COMMAND ${CMAKE_COMMAND} -E false)
endif()

# --------------------------------------------------------------------------
# Packaging
# --------------------------------------------------------------------------
include(sources/package/package.cmake)
