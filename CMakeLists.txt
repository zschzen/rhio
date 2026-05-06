# ============================================================================
# rhio — main build configuration
# ============================================================================
# Configure: cmake -B build
# Build:     cmake --build build
# Install:   cmake --install build
#
# See https://cmake.org/documentation for documentation.
# ============================================================================

cmake_minimum_required(VERSION 3.26..4.3 FATAL_ERROR)
project(
  rhio
  VERSION 0.0.1
  DESCRIPTION "Single-file header-only C99 RHI"
  LANGUAGES C
  HOMEPAGE_URL "https://github.com/zschzen/rhio"
)

message(STATUS "")
message(STATUS "~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=")
message(STATUS "   ▌ ▘   v${PROJECT_VERSION}")
message(STATUS " ▛▘▛▌▌▛▌ ${PROJECT_DESCRIPTION}")
message(STATUS " ▌ ▌▌▌▙▌ zlib/libpng licensed")
message(STATUS "~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=")
message(STATUS "")

# Determine if rhio is the main project
set(RHIO_IS_MAIN ${PROJECT_IS_TOP_LEVEL})

# --------------------------------------------------------------------
# Paths
# --------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --------------------------------------------------------------------
# Platform Detection
# --------------------------------------------------------------------
if(EMSCRIPTEN)
  set(PLATFORM Web CACHE STRING "Select the target platform for the build." FORCE)
endif()

# --------------------------------------------------------------------
# Options
# --------------------------------------------------------------------
include(CMakeOptions.cmake)

# --------------------------------------------------------------------
# Build Configuration
# --------------------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

if(MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------
message(STATUS "> Depedencies: CPM.cmake")
message(STATUS "")

include(cmake/CPM.cmake)
CPMUsePackageLock(package-lock.cmake)

CPMAddPackage("gh:cpm-cmake/CPMLicenses.cmake@0.0.7")
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")
CPMAddPackage("gh:TheLartians/GroupSourcesByFolder.cmake@1.0")

include(cmake/tools.cmake)

# --------------------------------------------------------------------
# Global Properties
# --------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --------------------------------------------------------------------
# Header-only Library
# --------------------------------------------------------------------
set(RHIO_PUBLIC_HEADER_FILES "${PROJECT_SOURCE_DIR}/rhio.h")

add_library(${PROJECT_NAME} INTERFACE ${RHIO_PUBLIC_HEADER_FILES})

# --------------------------------------------------------------------
# Compilation Features
# --------------------------------------------------------------------
target_compile_features(${PROJECT_NAME} INTERFACE c_std_99)
target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# --------------------------------------------------------------------
# Definitions
# --------------------------------------------------------------------
target_compile_definitions(
  ${PROJECT_NAME}
  INTERFACE
    # Build Type
    $<$<CONFIG:Debug>:RHIO_DEBUG>
    $<$<CONFIG:Release>:RHIO_RELEASE>

    # Platform
    "RHIO_PLATFORM_$<UPPER_CASE:${PLATFORM}>"

    # Backends
    $<$<BOOL:${RHIO_BACKEND_OPENGL}>:RHIO_BACKEND_OPENGL>
    $<$<BOOL:${RHIO_BACKEND_VULKAN}>:RHIO_BACKEND_VULKAN>

    # Log
    $<$<BOOL:${LOG_SUPPORT}>:RHIO_LOG_SUPPORT>
)

# --------------------------------------------------------------------
# Backend Dependencies
# --------------------------------------------------------------------
set(RHIO_BACKEND_LIBRARIES "")
set(RHIO_PACKAGE_DEPENDENCIES "")

if(NOT EMSCRIPTEN)
  if(RHIO_BACKEND_OPENGL)
    find_package(OpenGL REQUIRED)
    list(APPEND RHIO_BACKEND_LIBRARIES OpenGL::GL)
  endif()

  if(RHIO_BACKEND_VULKAN)
    find_package(Vulkan 1.4 REQUIRED)
    include(cmake/deps/volk.cmake)
    list(APPEND RHIO_BACKEND_LIBRARIES Vulkan::Headers volk::volk_headers)
    list(APPEND RHIO_PACKAGE_DEPENDENCIES "Vulkan 1.4")
    if(RHIO_VOLK_FROM_SYSTEM)
      list(APPEND RHIO_PACKAGE_DEPENDENCIES "volk")
    endif()
  endif()

endif()

if(RHIO_BACKEND_LIBRARIES)
  target_link_libraries(${PROJECT_NAME} INTERFACE ${RHIO_BACKEND_LIBRARIES})
endif()

# --------------------------------------------------------------------
# Installation Configuration
# --------------------------------------------------------------------
packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES ${RHIO_PACKAGE_DEPENDENCIES}
)

install(
  FILES ${RHIO_PUBLIC_HEADER_FILES}
  DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  COMPONENT "${PROJECT_NAME}_Development"
)

# --------------------------------------------------------------------
# Subdirectories
# --------------------------------------------------------------------
if(RHIO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# --------------------------------------------------------------------
# License collector
# --------------------------------------------------------------------
# Usage: --build build --target write-licenses
cpm_licenses_create_disclaimer_target(
  write-licenses "${CMAKE_CURRENT_BINARY_DIR}/third_party.txt" "${CPM_PACKAGES}"
)

# --------------------------------------------------------------------
# Group by folder
# --------------------------------------------------------------------
GroupSourcesByFolder(${PROJECT_NAME})

message(STATUS "")
message(STATUS "END ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=")
message(STATUS "")
