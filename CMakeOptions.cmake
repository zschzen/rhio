# ============================================================================
# rhio - build options
# ============================================================================

include(EnumOption)

# --------------------------------------------------------------------
# Define Enum Options
# --------------------------------------------------------------------
enum_option(PLATFORM "Desktop;Web" "Select the target platform for the build.")
enum_option(OPENGL_VERSION "Auto;3.3;ES 2.0;ES 3.0" "Select the OpenGL/OpenGL ES version.")

# --------------------------------------------------------------------
# Build Options
# --------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries for dependencies that honor BUILD_SHARED_LIBS" OFF)
option(RHIO_BUILD_EXAMPLES "Build rhio examples" ${RHIO_IS_MAIN})
option(RHIO_BUILD_TESTS "Build rhio unit tests" ${RHIO_IS_MAIN})

# Ccache
option(USE_CCACHE "Enable compiler cache that can drastically improve build times" ${RHIO_IS_MAIN})
option(CCACHE_OPTIONS "Compiler cache options" "CCACHE_CPP2=true;CCACHE_SLOPPINESS=clang_index_store")

# Log
option(LOG_SUPPORT "Enable rhio logging system" ON)

# Backends
option(RHIO_BACKEND_OPENGL "Enable rhio OpenGL/OpenGL ES backend" ON)
option(RHIO_BACKEND_VULKAN "Enable rhio Vulkan backend" OFF)

# --------------------------------------------------------------------
# Sanitize Options
# --------------------------------------------------------------------
option(SANITIZE_ADDRESS "Enable Address sanitizer" OFF)
option(SANITIZE_MEMORY "Enable Memory sanitizer" OFF)
option(SANITIZE_THREAD "Enable Thread sanitizer" OFF)
option(SANITIZE_UNDEFINED "Enable Undefined Behavior sanitizer" OFF)
option(SANITIZE_LINK_STATIC "Link sanitizers statically (for gcc)" OFF)
