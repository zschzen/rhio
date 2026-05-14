# rhio Unit Tests

## Testing

Configure and run the unit tests from the repository root:

```sh
cmake -S . -B build-tests -DRHIO_BUILD_TESTS=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

CTest registers each `TEST(suite, case)` as an individual test, so failures are
reported at the smallest useful level. For native colored `rktest` output, run:

```sh
cmake --build build-tests --target rhio_unit_report
```

Add new C unit test files with the `*_tests.c` suffix. The unit CMake project
discovers those files automatically, links them into `rhio_unit_tests`, and scans
their `TEST(...)` declarations for CTest registration.
