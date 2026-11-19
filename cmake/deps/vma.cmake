if(NOT EMSCRIPTEN)
  string(TIMESTAMP BEFORE "%s")

  message(STATUS "")
  message(STATUS "========================================")
  message(STATUS "+ Vulkan Memory Allocator")
  message(STATUS "")

  set(VMA_VERSION v3.5.0)
  set(RHIO_VMA_FROM_SYSTEM OFF)

  find_package(VulkanMemoryAllocator QUIET)

  if(NOT TARGET GPUOpen::VulkanMemoryAllocator)
    message(STATUS "! Vulkan Memory Allocator: system package not found, using CPM.cmake")
    CPMAddPackage(
      NAME VulkanMemoryAllocator
      GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
      GIT_TAG ${VMA_VERSION}
      OPTIONS
        "VMA_BUILD_SAMPLES OFF"
        "VMA_BUILD_TESTS OFF"
    )
  else()
    message(STATUS "! Vulkan Memory Allocator: found package")
  endif()

  if(NOT TARGET GPUOpen::VulkanMemoryAllocator)
    message(FATAL_ERROR "Failed to find or add Vulkan Memory Allocator")
  endif()

  get_target_property(RHIO_VMA_IMPORTED GPUOpen::VulkanMemoryAllocator IMPORTED)
  if(RHIO_VMA_IMPORTED)
    set(RHIO_VMA_FROM_SYSTEM ON)
  endif()

  # Bundle the header-only target into the rhio export set so downstream
  # find_package(rhio) resolves the dependency like volk_headers does
  if(TARGET VulkanMemoryAllocator)
    install(
      TARGETS VulkanMemoryAllocator
      EXPORT ${PROJECT_NAME}Targets
      COMPONENT "${PROJECT_NAME}_Development"
    )
  endif()

  string(TIMESTAMP AFTER "%s")
  math(EXPR DELTA_VMA "${AFTER} - ${BEFORE}")

  message(STATUS "")
  message(STATUS "+ Vulkan Memory Allocator in ${DELTA_VMA}s")
  message(STATUS "========================================")
  message(STATUS "")
endif()
