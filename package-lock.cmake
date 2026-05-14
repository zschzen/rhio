# CPM Package Lock
# This file should be committed to version control

# Ccache.cmake
CPMDeclarePackage(Ccache.cmake
  VERSION 1.2.5
  GITHUB_REPOSITORY TheLartians/Ccache.cmake
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

# cmake-scripts
CPMDeclarePackage(cmake-scripts
  NAME cmake-scripts
  GIT_TAG 24.04.1
  GITHUB_REPOSITORY StableCoder/cmake-scripts
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

# CPMLicenses.cmake
CPMDeclarePackage(CPMLicenses.cmake
  VERSION 0.0.7
  GITHUB_REPOSITORY cpm-cmake/CPMLicenses.cmake
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

# PackageProject.cmake
CPMDeclarePackage(PackageProject.cmake
  VERSION 1.13.0
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

# GroupSourcesByFolder.cmake
CPMDeclarePackage(GroupSourcesByFolder.cmake
  VERSION 1.0
  GITHUB_REPOSITORY TheLartians/GroupSourcesByFolder.cmake
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
)

# rktest
CPMDeclarePackage(rktest
  NAME rktest
  GIT_TAG 6d8f9354bfff2bfd0944ffd29264ed6058dcaa5b
  GITHUB_REPOSITORY Warwolt/rktest
  SYSTEM YES
  EXCLUDE_FROM_ALL YES
  OPTIONS
    "rktest_build_tests OFF"
    "rktest_build_samples OFF"
)

# GLFW
CPMDeclarePackage(GLFW
  NAME GLFW
  GIT_TAG 3.4
  GITHUB_REPOSITORY glfw/glfw
  OPTIONS
    "GLFW_BUILD_DOCS OFF"
    "GLFW_BUILD_TESTS OFF"
    "GLFW_BUILD_EXAMPLES OFF"
    "GLFW_INSTALL ON"
)

# Volk
CPMDeclarePackage(volk
  NAME volk
  GIT_TAG vulkan-sdk-1.4.341.0
  GITHUB_REPOSITORY zeux/volk
  OPTIONS
    "VOLK_HEADERS_ONLY ON"
    "VOLK_PULL_IN_VULKAN ON"
    "VOLK_INSTALL OFF"
)
