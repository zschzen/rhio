# --------------------------------------------------------------------
# rhio Benchmark Suite
# --------------------------------------------------------------------
# CPU-side API overhead benchmarks over the Vulkan backend. These run
# headless (device without a surface) and exercise creation/destroy and
# record paths rather than GPU frame throughput.
#
# Google benchmark provides the selection knobs:
#   ./rhio_benchmarks --benchmark_filter=Buffer
#   ./rhio_benchmarks --benchmark_min_time=2s
#   ./rhio_benchmarks --benchmark_repetitions=5
# --------------------------------------------------------------------

project(rhio_benchmarks LANGUAGES C CXX)

CPMAddPackage(NAME benchmark)

# GLFW drives the windowed frame benchmark on both backends
CPMAddPackage(NAME GLFW)

if(RHIO_BACKEND_VULKAN)
  add_executable(rhio_benchmarks
    vulkan_bench.cpp
    impl_vk.cpp
  )

  target_compile_features(rhio_benchmarks PRIVATE cxx_std_20)
  target_link_libraries(rhio_benchmarks PRIVATE rhio::rhio benchmark::benchmark)

  add_executable(rhio_bench_vk
    frame_bench.cpp
    impl_vk.cpp
  )
  target_compile_features(rhio_bench_vk PRIVATE cxx_std_20)
  target_link_libraries(rhio_bench_vk PRIVATE rhio::rhio benchmark::benchmark glfw)
endif()

if(RHIO_BACKEND_OPENGL)
  add_executable(rhio_bench_gl
    frame_bench.cpp
    impl_gl.c
  )
  set_property(SOURCE impl_gl.c PROPERTY COMPILE_DEFINITIONS RHIO_IMPLEMENTATION RHIO_BACKEND_OPENGL)
  set_property(SOURCE impl_gl.c PROPERTY COMPILE_OPTIONS -std=c99)
  target_compile_features(rhio_bench_gl PRIVATE cxx_std_20)
  target_link_libraries(rhio_bench_gl PRIVATE rhio::rhio benchmark::benchmark glfw)
endif()
