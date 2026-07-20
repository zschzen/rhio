if(NOT EMSCRIPTEN AND NOT APPLE)
  string(TIMESTAMP BEFORE "%s")

  message(STATUS "")
  message(STATUS "========================================")
  message(STATUS "+ GLEW")
  message(STATUS "")

  set(GLEW_VERSION glew-cmake-2.2.0)
  set(RHIO_GLEW_FROM_SYSTEM OFF)
  set(RHIO_GLEW_TARGET "")

  set(GLEW_USE_STATIC_LIBS ON)
  find_package(GLEW QUIET)

  if(TARGET GLEW::GLEW)
    set(RHIO_GLEW_TARGET GLEW::GLEW)
  elseif(TARGET GLEW::glew_s)
    set(RHIO_GLEW_TARGET GLEW::glew_s)
  elseif(TARGET libglew_static)
    set(RHIO_GLEW_TARGET libglew_static)
  endif()

  if(NOT RHIO_GLEW_TARGET)
    message(STATUS "! GLEW: system package not found, using CPM.cmake")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
      if(DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(RHIO_GLEW_OLD_CMAKE_POLICY_VERSION_MINIMUM "${CMAKE_POLICY_VERSION_MINIMUM}")
        set(RHIO_GLEW_HAD_CMAKE_POLICY_VERSION_MINIMUM ON)
      else()
        set(RHIO_GLEW_HAD_CMAKE_POLICY_VERSION_MINIMUM OFF)
      endif()
      set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()

    CPMAddPackage(
      NAME glew
      GITHUB_REPOSITORY Perlmint/glew-cmake
      GIT_TAG ${GLEW_VERSION}
      OPTIONS
        "glew-cmake_BUILD_SHARED OFF"
        "glew-cmake_BUILD_STATIC ON"
        "ONLY_LIBS ON"
        "USE_GLU OFF"
    )

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
      if(RHIO_GLEW_HAD_CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM "${RHIO_GLEW_OLD_CMAKE_POLICY_VERSION_MINIMUM}")
      else()
        unset(CMAKE_POLICY_VERSION_MINIMUM)
      endif()
      unset(RHIO_GLEW_OLD_CMAKE_POLICY_VERSION_MINIMUM)
      unset(RHIO_GLEW_HAD_CMAKE_POLICY_VERSION_MINIMUM)
    endif()
  else()
    message(STATUS "! GLEW: found system package")
  endif()

  if(NOT RHIO_GLEW_TARGET AND TARGET libglew_static)
    set(RHIO_GLEW_TARGET libglew_static)
  endif()

  if(NOT RHIO_GLEW_TARGET)
    message(FATAL_ERROR "Failed to find or add GLEW")
  endif()

  get_target_property(RHIO_GLEW_IMPORTED ${RHIO_GLEW_TARGET} IMPORTED)
  if(RHIO_GLEW_IMPORTED)
    set(RHIO_GLEW_FROM_SYSTEM ON)
  endif()

  if(NOT RHIO_GLEW_FROM_SYSTEM AND TARGET libglew_static)
    if(UNIX AND NOT APPLE)
      list(APPEND RHIO_PACKAGE_DEPENDENCIES "X11")
    endif()

    install(
      TARGETS libglew_static
      EXPORT ${PROJECT_NAME}Targets
      COMPONENT "${PROJECT_NAME}_Development"
    )

    if(DEFINED glew_SOURCE_DIR)
      if(EXISTS "${glew_SOURCE_DIR}/include/GL")
        install(
          DIRECTORY "${glew_SOURCE_DIR}/include/GL"
          DESTINATION include
          COMPONENT "${PROJECT_NAME}_Development"
          FILES_MATCHING PATTERN "*.h"
        )
      endif()

      if(EXISTS "${glew_SOURCE_DIR}/include/KHR")
        install(
          DIRECTORY "${glew_SOURCE_DIR}/include/KHR"
          DESTINATION include
          COMPONENT "${PROJECT_NAME}_Development"
          FILES_MATCHING PATTERN "*.h"
        )
      endif()
    endif()
  endif()

  string(TIMESTAMP AFTER "%s")
  math(EXPR DELTA_GLEW "${AFTER} - ${BEFORE}")

  message(STATUS "")
  message(STATUS "+ GLEW in ${DELTA_GLEW}s")
  message(STATUS "========================================")
  message(STATUS "")
endif()
