# --------------------------------------------------------------------
# Project Setup
# --------------------------------------------------------------------
cmake_minimum_required(VERSION 3.26)
project(rhio_Example LANGUAGES C CXX)

# --------------------------------------------------------------------
# Output Directories
# --------------------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# --------------------------------------------------------------------
# Platform Defaults
# --------------------------------------------------------------------
if(EMSCRIPTEN)
  set(PLATFORM Web CACHE STRING "Select the target platform for the build." FORCE)
endif()

# --------------------------------------------------------------------
# CMake Helpers
# --------------------------------------------------------------------
include(../cmake/CPM.cmake)
include(../cmake/tools.cmake)

# --------------------------------------------------------------------
# Graphics API Selection
# --------------------------------------------------------------------
if(RHIO_BACKEND_VULKAN AND DEFINED RHIO_BACKEND_OPENGL AND NOT RHIO_BACKEND_OPENGL)
  set(RHIO_EXAMPLE_DEFAULT_GRAPHICS_API Vulkan)
else()
  set(RHIO_EXAMPLE_DEFAULT_GRAPHICS_API OpenGL)
endif()

set(RHIO_EXAMPLE_GRAPHICS_API "${RHIO_EXAMPLE_DEFAULT_GRAPHICS_API}" CACHE STRING "Select the graphics API used by examples.")
set_property(CACHE RHIO_EXAMPLE_GRAPHICS_API PROPERTY STRINGS OpenGL Vulkan)

option(RHIO_EXAMPLE_SITE_BUNDLE "Emscripten only: emit JS+WASM (no HTML shell) and stage into build/site for the Pages deploy." OFF)

if(NOT RHIO_EXAMPLE_GRAPHICS_API MATCHES "^(OpenGL|Vulkan)$")
  message(FATAL_ERROR "RHIO_EXAMPLE_GRAPHICS_API must be OpenGL or Vulkan.")
endif()

if(RHIO_EXAMPLE_GRAPHICS_API STREQUAL "Vulkan")
  if(EMSCRIPTEN)
    message(FATAL_ERROR "RHIO_EXAMPLE_GRAPHICS_API=Vulkan is not supported on Emscripten.")
  endif()

  if(NOT DEFINED RHIO_BACKEND_VULKAN)
    set(RHIO_BACKEND_VULKAN ON CACHE BOOL "Enable rhio Vulkan backend")
  endif()
endif()

# --------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------
if(NOT TARGET rhio::rhio)
  CPMAddPackage(NAME rhio SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

CPMAddPackage("gh:TheLartians/GroupSourcesByFolder.cmake@1.0")

# --------------------------------------------------------------------
# Graphics API Validation
# --------------------------------------------------------------------
if(RHIO_EXAMPLE_GRAPHICS_API STREQUAL "Vulkan" AND NOT RHIO_BACKEND_VULKAN)
  message(FATAL_ERROR "RHIO_EXAMPLE_GRAPHICS_API=Vulkan requires RHIO_BACKEND_VULKAN=ON.")
endif()

if(RHIO_EXAMPLE_GRAPHICS_API STREQUAL "OpenGL" AND DEFINED RHIO_BACKEND_OPENGL AND NOT RHIO_BACKEND_OPENGL)
  message(FATAL_ERROR "RHIO_EXAMPLE_GRAPHICS_API=OpenGL requires RHIO_BACKEND_OPENGL=ON.")
endif()

message(STATUS "! Examples graphics API: ${RHIO_EXAMPLE_GRAPHICS_API}")

# --------------------------------------------------------------------
# Link Libraries
# --------------------------------------------------------------------
set(RHIO_EXAMPLE_LINK_LIBRARIES rhio::rhio)

if(NOT EMSCRIPTEN)
  include(../cmake/deps/glfw.cmake)
  list(APPEND RHIO_EXAMPLE_LINK_LIBRARIES glfw)
endif()

# --------------------------------------------------------------------
# Backend Compile Definitions
# --------------------------------------------------------------------
set(RHIO_EXAMPLE_COMPILE_DEFINITIONS)

if(RHIO_EXAMPLE_GRAPHICS_API STREQUAL "Vulkan")
  list(APPEND RHIO_EXAMPLE_COMPILE_DEFINITIONS
    RHIO_BACKEND_VULKAN
    RHIO_EXAMPLE_BACKEND_VULKAN
  )
else()
  list(APPEND RHIO_EXAMPLE_COMPILE_DEFINITIONS RHIO_EXAMPLE_BACKEND_OPENGL)

  if(OPENGL_VERSION STREQUAL "ES 2.0")
    set(RHIO_WEBGL_VERSION 1)
    list(APPEND RHIO_EXAMPLE_COMPILE_DEFINITIONS
      RHIO_BACKEND_OPENGLES2
      RHIO_EXAMPLE_OPENGL_ES_VERSION=2
    )
  elseif(OPENGL_VERSION STREQUAL "ES 3.0" OR EMSCRIPTEN)
    set(RHIO_WEBGL_VERSION 2)
    list(APPEND RHIO_EXAMPLE_COMPILE_DEFINITIONS
      RHIO_BACKEND_OPENGLES3
      RHIO_EXAMPLE_OPENGL_ES_VERSION=3
    )
  else()
    list(APPEND RHIO_EXAMPLE_COMPILE_DEFINITIONS RHIO_BACKEND_OPENGL)

    if(OPENGL_VERSION STREQUAL "3.3")
      list(APPEND RHIO_EXAMPLE_COMPILE_DEFINITIONS RHIO_EXAMPLE_OPENGL_3_3)
    endif()
  endif()
endif()

# --------------------------------------------------------------------
# Shared Example Sources
# --------------------------------------------------------------------
set(RHIO_EXAMPLE_SHARED_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/example.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/window.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/window.hpp"
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/window.cpp"
    PROPERTIES COMPILE_DEFINITIONS RHIO_IMPLEMENTATION
)

# --------------------------------------------------------------------
# Example Discovery
# --------------------------------------------------------------------
file(GLOB_RECURSE RHIO_EXAMPLE_SOURCE_FILES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cxx"
)
list(SORT RHIO_EXAMPLE_SOURCE_FILES)

# --------------------------------------------------------------------
# Example Targets
# --------------------------------------------------------------------
foreach(SOURCE_FILE IN LISTS RHIO_EXAMPLE_SOURCE_FILES)
  get_filename_component(EXECUTABLE_NAME ${SOURCE_FILE} NAME_WE)
  file(RELATIVE_PATH SOURCE_FILE_RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${SOURCE_FILE}")
  get_filename_component(EXAMPLE_GROUP ${SOURCE_FILE_RELATIVE} DIRECTORY)

  if(TARGET ${EXECUTABLE_NAME})
    message(FATAL_ERROR "Duplicate example target '${EXECUTABLE_NAME}' from '${SOURCE_FILE_RELATIVE}'. Rename one of the examples.")
  endif()

  add_executable(${EXECUTABLE_NAME}
    ${RHIO_EXAMPLE_SHARED_FILES}
    ${SOURCE_FILE}
  )

  target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${RHIO_EXAMPLE_LINK_LIBRARIES})
  target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_20)
  target_compile_definitions(${EXECUTABLE_NAME} PRIVATE ${RHIO_EXAMPLE_COMPILE_DEFINITIONS})

  # --------------------------------------------------------------------
  # Windows Target Settings
  # --------------------------------------------------------------------
  if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})
    set_property(TARGET ${EXECUTABLE_NAME} PROPERTY WIN32_EXECUTABLE true)
    target_link_options(${EXECUTABLE_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
  endif()

  # --------------------------------------------------------------------
  # Emscripten Target Settings
  # --------------------------------------------------------------------
  if(EMSCRIPTEN)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:-O3>)
    target_link_options(
      ${EXECUTABLE_NAME}
      PRIVATE
        "SHELL:-s USE_GLFW=3"
        "SHELL:-s ASSERTIONS=1"
        "SHELL:-s WASM=1"
        "SHELL:-s ASYNCIFY"
        "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH"
        "SHELL:-s MIN_WEBGL_VERSION=${RHIO_WEBGL_VERSION}"
        "SHELL:-s MAX_WEBGL_VERSION=${RHIO_WEBGL_VERSION}"
        $<$<CONFIG:Release>:-O3>
    )

    if(RHIO_EXAMPLE_SITE_BUNDLE)
      # Site mode: JS+WASM loader only, staged into the Pages tree
      set_target_properties(${EXECUTABLE_NAME} PROPERTIES SUFFIX ".js")

      set(_site_bin_dir "${CMAKE_BINARY_DIR}/site/examples/bin")
      set(_site_src_dir "${CMAKE_BINARY_DIR}/site/examples/src")

      add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_site_bin_dir}" "${_site_src_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/${EXECUTABLE_NAME}.js"
          "${_site_bin_dir}/${EXECUTABLE_NAME}.js"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/${EXECUTABLE_NAME}.wasm"
          "${_site_bin_dir}/${EXECUTABLE_NAME}.wasm"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${SOURCE_FILE}"
          "${_site_src_dir}/${EXECUTABLE_NAME}.cpp"
        COMMENT "Staging ${EXECUTABLE_NAME} into site bundle"
        VERBATIM
      )

      list(APPEND RHIO_SITE_EXAMPLE_TARGETS ${EXECUTABLE_NAME})
    else()
      # Standalone example mode: full HTML shell + emrun harness
      target_link_options(${EXECUTABLE_NAME} PRIVATE --emrun)
      set_target_properties(${EXECUTABLE_NAME} PROPERTIES SUFFIX ".html")
    endif()
  endif()

  # --------------------------------------------------------------------
  # IDE Organization
  # --------------------------------------------------------------------
  if(EXAMPLE_GROUP)
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "examples/${EXAMPLE_GROUP}")
  endif()

  GroupSourcesByFolder(${EXECUTABLE_NAME})
endforeach()

# --------------------------------------------------------------------
# Pages Site Aggregate Target
# --------------------------------------------------------------------
if(EMSCRIPTEN AND RHIO_SITE_EXAMPLE_TARGETS)
  add_custom_target(rhio_site_examples
    DEPENDS ${RHIO_SITE_EXAMPLE_TARGETS}
    COMMENT "Building all rhio examples for the Pages site"
  )
endif()
