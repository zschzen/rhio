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

add_executable(rhio_benchmarks
  vulkan_bench.cpp
)

# In-file defines keep the TU self-contained for standalone compiles
target_compile_features(rhio_benchmarks PRIVATE cxx_std_20)
target_link_libraries(rhio_benchmarks PRIVATE rhio::rhio benchmark::benchmark)
