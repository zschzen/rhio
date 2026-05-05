# --------------------------------------------------------------------
# Ccache
# ------------------------------------------------------------------
if(USE_CCACHE)
  CPMAddPackage("gh:TheLartians/Ccache.cmake@1.2.5")
endif()

# --------------------------------------------------------------------
# Sanitizers
# ------------------------------------------------------------------
set(RHIO_REQUESTED_SANITIZERS "")

if(SANITIZE_ADDRESS)
  list(APPEND RHIO_REQUESTED_SANITIZERS "Address")
endif()

if(SANITIZE_MEMORY)
  list(APPEND RHIO_REQUESTED_SANITIZERS "Memory")
endif()

if(SANITIZE_THREAD)
  list(APPEND RHIO_REQUESTED_SANITIZERS "Thread")
endif()

if(SANITIZE_UNDEFINED)
  list(APPEND RHIO_REQUESTED_SANITIZERS "Undefined")
endif()

if(RHIO_REQUESTED_SANITIZERS AND NOT USE_SANITIZER)
  set(USE_SANITIZER "${RHIO_REQUESTED_SANITIZERS}" CACHE STRING "Enable sanitizers" FORCE)
endif()

set(USE_STATIC_ANALYZER "" CACHE STRING "Enable static analyzers")

if(USE_SANITIZER OR USE_STATIC_ANALYZER)
  CPMAddPackage("gh:StableCoder/cmake-scripts#24.04.1")
  if(USE_SANITIZER)
    include(${cmake-scripts_SOURCE_DIR}/sanitizers.cmake)
  endif()
  if(USE_STATIC_ANALYZER)
    if("clang-tidy" IN_LIST USE_STATIC_ANALYZER)
      set(CLANG_TIDY
          ON
          CACHE INTERNAL ""
      )
    else()
      set(CLANG_TIDY
          OFF
          CACHE INTERNAL ""
      )
    endif()
    if("iwyu" IN_LIST USE_STATIC_ANALYZER)
      set(IWYU
          ON
          CACHE INTERNAL ""
      )
    else()
      set(IWYU
          OFF
          CACHE INTERNAL ""
      )
    endif()
    if("cppcheck" IN_LIST USE_STATIC_ANALYZER)
      set(CPPCHECK
          ON
          CACHE INTERNAL ""
      )
    else()
      set(CPPCHECK
          OFF
          CACHE INTERNAL ""
      )
    endif()
    include(${cmake-scripts_SOURCE_DIR}/tools.cmake)
    if(${CLANG_TIDY})
      clang_tidy(${CLANG_TIDY_ARGS})
    endif()
    if(${IWYU})
      include_what_you_use(${IWYU_ARGS})
    endif()
    if(${CPPCHECK})
      cppcheck(${CPPCHECK_ARGS})
    endif()
  endif()
endif()
