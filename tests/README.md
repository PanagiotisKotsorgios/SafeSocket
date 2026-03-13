# SafeSocket - Tests

This directory contains the full test suite for SafeSocket, organised into three tiers: **unit**, **integration**, and **stress** tests.

> **Total: ~141 tests across 16 suites.**

---

## Building

### CMake (recommended — Linux, macOS, Windows)

```bash
cd tests
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
```

Run all tests via CTest:

```bash
ctest --output-on-failure
```

Run only a labelled subset:

```bash
ctest -L unit        --output-on-failure
ctest -L integration --output-on-failure
ctest -L stress      --output-on-failure
```

Enable sanitisers:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSS_ASAN=ON   # AddressSanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSS_TSAN=ON   # ThreadSanitizer
```

### GNU Make (Linux / macOS)

```bash
cd tests
make -j$(nproc)           # build everything
make unit                 # build + run unit tests
make integration          # build + run integration tests
make stress               # build + run stress tests
make all_tests            # run all three tiers in sequence
make ASAN=1 unit          # unit tests with AddressSanitizer
make clean
```

### Windows (MSVC, no CMake)

```bat
cd tests
cl /std:c++17 /EHsc /W3 /I. /I.. ^
   ..\crypto.cpp ..\network.cpp ..\config.cpp ..\server.cpp ..\client.cpp ^
   unit\crypto\test_xor.cpp ^
   /Fe:test_xor.exe /link ws2_32.lib
test_xor.exe
```

Repeat for each test file, substituting the source path and output name.

---

## Test Framework

`helpers/test_framework.hpp` is a zero-dependency, single-header mini test runner. No external libraries (Google Test, Catch2, etc.) are required.

| Macro | Meaning |
|---|---|
| `TEST(Suite, Name)` | Declare a test case |
| `ASSERT_TRUE(expr)` | Fail if `expr` is false |
| `ASSERT_FALSE(expr)` | Fail if `expr` is true |
| `ASSERT_EQ(a, b)` | Fail if `a != b` |
| `ASSERT_NE(a, b)` | Fail if `a == b` |
| `ASSERT_LT/LE/GT/GE(a, b)` | Ordered comparisons |
| `ASSERT_STREQ(a, b)` | String equality |
| `ASSERT_STRNE(a, b)` | String inequality |
| `RUN_ALL_TESTS()` | Run every registered test, return exit code |

Every test file has its own `main()` that calls `RUN_ALL_TESTS()`, so each binary is independently executable.

---

## Test Tier Guide

### Unit tests

Pure in-process tests. No sockets, no files (except `test_config_load_save`). Fast — the entire unit suite completes in under a second on any modern machine.

### Integration tests

Spin up a real `Server` on a random loopback port and connect real `Client` instances. Each test starts and stops its own server so tests are fully isolated. These take a few seconds due to `sleep_ms()` calls that let threads settle.

### Stress tests

Designed to catch race conditions, resource leaks, and deadlocks under load. They are intentionally slow (up to 120 s timeout in CTest). Run them separately in CI or as a nightly job:

```bash
ctest -L stress --timeout 120 --output-on-failure
```
---

## Adding a New Test

1. Create a `.cpp` file in the appropriate subdirectory.
2. Include `helpers/test_framework.hpp` and the header(s) under test.
3. Write `TEST(Suite, CaseName) { ... }` blocks using the assertion macros.
4. Add `int main() { return RUN_ALL_TESTS(); }` at the bottom.
5. Add the new binary to `CMakeLists.txt` (`ss_test(...)`) and `Makefile` (`ALL_BINS` or the relevant tier list).

---

## Known Limitations

- Integration and stress tests require that the host has loopback (`127.0.0.1`) available.
- `test_access_key` and `test_file_send` modify the global `g_config` singleton. Tests within those files are designed to restore state after each case.
- Stress tests are inherently timing-sensitive. Flakiness under extreme system load is expected; re-run before filing a bug.
- The test framework does not support test fixtures (setUp/tearDown). Use local helper lambdas or RAII objects instead.
