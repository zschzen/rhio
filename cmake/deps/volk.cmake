if(NOT EMSCRIPTEN)
  string(TIMESTAMP BEFORE "%s")

  message(STATUS "")
  message(STATUS "========================================")
  message(STATUS "+ Volk")
  message(STATUS "")

  set(VOLK_VERSION vulkan-sdk-1.4.341.0)
  set(RHIO_VOLK_FROM_SYSTEM OFF)

  find_package(volk QUIET)

  if(NOT TARGET volk::volk_headers)
    message(STATUS "! Volk: system package not found, using CPM.cmake")
    CPMAddPackage(
      NAME volk
      GITHUB_REPOSITORY zeux/volk
      GIT_TAG ${VOLK_VERSION}
      OPTIONS
        "VOLK_HEADERS_ONLY ON"
        "VOLK_PULL_IN_VULKAN ON"
        "VOLK_INSTALL OFF"
    )
  else()
    message(STATUS "! Volk: found package")
  endif()

  if(NOT TARGET volk::volk_headers)
    message(FATAL_ERROR "Failed to find or add Volk")
  endif()

  get_target_property(RHIO_VOLK_IMPORTED volk::volk_headers IMPORTED)
  if(RHIO_VOLK_IMPORTED)
    set(RHIO_VOLK_FROM_SYSTEM ON)
  endif()

  if(NOT RHIO_VOLK_FROM_SYSTEM AND TARGET volk_headers)
    install(
      TARGETS volk_headers
      EXPORT ${PROJECT_NAME}Targets
      COMPONENT "${PROJECT_NAME}_Development"
    )

    if(DEFINED volk_SOURCE_DIR AND EXISTS "${volk_SOURCE_DIR}/volk.h")
      install(
        FILES "${volk_SOURCE_DIR}/volk.h"
        DESTINATION include
        COMPONENT "${PROJECT_NAME}_Development"
      )
    endif()
  endif()

  string(TIMESTAMP AFTER "%s")
  math(EXPR DELTA_VOLK "${AFTER} - ${BEFORE}")

  message(STATUS "")
  message(STATUS "+ Volk in ${DELTA_VOLK}s")
  message(STATUS "========================================")
  message(STATUS "")
endif()
