if(NOT EMSCRIPTEN)
  string(TIMESTAMP BEFORE "%s")

  message(STATUS "")
  message(STATUS "========================================")
  message(STATUS "+ Vulkan-Headers")
  message(STATUS "")

  # Pinned unconditionally so the header surface stays identical on every
  # machine, including hosts that only ship a Vulkan loader (rhio loads Vulkan
  # through volk at runtime and never links the loader library)
  set(RHIO_VULKAN_HEADERS_VERSION vulkan-sdk-1.4.350.0)

  CPMAddPackage(
    NAME Vulkan-Headers
    GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
    GIT_TAG ${RHIO_VULKAN_HEADERS_VERSION}
  )

  if(NOT TARGET Vulkan::Headers)
    message(FATAL_ERROR "Failed to add Vulkan-Headers")
  endif()

  # Bundle the header-only target into the rhio export set so downstream
  # find_package(rhio) resolves the dependency like volk_headers does
  install(
    TARGETS Vulkan-Headers
    EXPORT ${PROJECT_NAME}Targets
    COMPONENT "${PROJECT_NAME}_Development"
  )

  string(TIMESTAMP AFTER "%s")
  math(EXPR DELTA_VULKAN_HEADERS "${AFTER} - ${BEFORE}")

  message(STATUS "")
  message(STATUS "+ Vulkan-Headers in ${DELTA_VULKAN_HEADERS}s")
  message(STATUS "========================================")
  message(STATUS "")
endif()
