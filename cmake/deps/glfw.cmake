if(NOT DEFINED EMSCRIPTEN)
  string(TIMESTAMP BEFORE "%s")

  message(STATUS "")
  message(STATUS "========================================")
  message(STATUS "+ GLFW")
  message(STATUS "")

  set(GLFW_VERSION 3.4)

  find_package(glfw3 ${GLFW_VERSION} QUIET)

  if(NOT glfw3_FOUND)
    message(STATUS "! GLFW: system package not found, using CPM.cmake")
    CPMAddPackage(
      NAME GLFW
      GITHUB_REPOSITORY glfw/glfw
      GIT_TAG ${GLFW_VERSION}
      OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_INSTALL ON"
    )
  else()
    message(STATUS "! GLFW: found system package")
  endif()

  if(NOT TARGET glfw)
    message(FATAL_ERROR "Failed to find or add GLFW")
  endif()

  string(TIMESTAMP AFTER "%s")
  math(EXPR DELTA_GLFW "${AFTER} - ${BEFORE}")

  message(STATUS "")
  message(STATUS "+ GLFW in ${DELTA_GLFW}s")
  message(STATUS "========================================")
  message(STATUS "")
endif()
