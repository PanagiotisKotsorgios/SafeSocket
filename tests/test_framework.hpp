/**
 * @file test_framework.hpp
 * @brief Minimal single-header test framework for SafeSocket.
 *
 * No external dependencies. Provides:
 *   - TEST(suite, name)  — declare a test case
 *   - ASSERT_TRUE / ASSERT_FALSE
 *   - ASSERT_EQ / ASSERT_NE
 *   - ASSERT_STREQ / ASSERT_STRNE
 *   - ASSERT_LT / ASSERT_LE / ASSERT_GT / ASSERT_GE
 *   - RUN_ALL_TESTS()    — run every registered test, return exit code
 */

#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <cstring>

// ── ANSI colours (disabled on Windows unless explicitly enabled) ──────────────
#ifdef _WIN32
  #define TC_RED    ""
  #define TC_GREEN  ""
  #define TC_YELLOW ""
  #define TC_RESET  ""
#else
  #define TC_RED    "\033[31m"
  #define TC_GREEN  "\033[32m"
  #define TC_YELLOW "\033[33m"
  #define TC_RESET  "\033[0m"
#endif

// ── Internal registry ─────────────────────────────────────────────────────────
namespace tf_internal {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

inline int& failure_count() { static int n = 0; return n; }
inline int& pass_count()    { static int n = 0; return n; }

inline void register_test(const char* suite, const char* name,
                           std::function<void()> fn) {
    registry().push_back({suite, name, fn});
}

struct AssertionFailure {
    std::string message;
};

} // namespace tf_internal

// ── Public assertion macros ───────────────────────────────────────────────────
#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_TRUE(" #expr ") failed"; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_FALSE(" #expr ") failed"; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a == _b)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_EQ(" #a ", " #b ") failed: " \
                << _a << " != " << _b; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a == _b) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_NE(" #a ", " #b ") failed: both equal " << _a; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_LT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a < _b)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_LT(" #a " < " #b ") failed: " << _a << " >= " << _b; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_LE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a <= _b)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_LE(" #a " <= " #b ") failed: " << _a << " > " << _b; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_GT(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a > _b)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_GT(" #a " > " #b ") failed: " << _a << " <= " << _b; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_GE(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (!(_a >= _b)) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_GE(" #a " >= " #b ") failed: " << _a << " < " << _b; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_STREQ(a, b) \
    do { \
        std::string _a = (a); std::string _b = (b); \
        if (_a != _b) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_STREQ failed: \"" << _a << "\" != \"" << _b << "\""; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

#define ASSERT_STRNE(a, b) \
    do { \
        std::string _a = (a); std::string _b = (b); \
        if (_a == _b) { \
            std::ostringstream _ss; \
            _ss << __FILE__ << ":" << __LINE__ \
                << "  ASSERT_STRNE failed: both equal \"" << _a << "\""; \
            throw tf_internal::AssertionFailure{_ss.str()}; \
        } \
    } while(0)

// ── TEST macro ────────────────────────────────────────────────────────────────
#define TEST(suite, name) \
    static void test_##suite##_##name(); \
    static bool _reg_##suite##_##name = []() -> bool { \
        tf_internal::register_test(#suite, #name, test_##suite##_##name); \
        return true; \
    }(); \
    static void test_##suite##_##name()

// ── RUN_ALL_TESTS ─────────────────────────────────────────────────────────────
inline int RUN_ALL_TESTS() {
    auto& reg = tf_internal::registry();
    int total   = (int)reg.size();
    int passed  = 0;
    int failed  = 0;

    std::cout << "\n[==========] Running " << total << " test(s)\n\n";

    std::string last_suite;
    for (auto& tc : reg) {
        if (tc.suite != last_suite) {
            std::cout << "[ " << TC_YELLOW << tc.suite << TC_RESET << " ]\n";
            last_suite = tc.suite;
        }
        std::cout << "  [ RUN  ] " << tc.name << "\n";
        try {
            tc.fn();
            std::cout << "  [" << TC_GREEN << " OK  " << TC_RESET << "] " << tc.name << "\n";
            ++passed;
        } catch (const tf_internal::AssertionFailure& e) {
            std::cout << "  [" << TC_RED << "FAIL " << TC_RESET << "] " << tc.name
                      << "\n         " << e.message << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "  [" << TC_RED << "EXCP " << TC_RESET << "] " << tc.name
                      << "\n         exception: " << e.what() << "\n";
            ++failed;
        } catch (...) {
            std::cout << "  [" << TC_RED << "EXCP " << TC_RESET << "] " << tc.name
                      << "\n         unknown exception\n";
            ++failed;
        }
    }

    std::cout << "\n[==========] " << total << " test(s) run\n";
    if (passed) std::cout << TC_GREEN << "[  PASSED  ] " << passed << " test(s)\n" << TC_RESET;
    if (failed) std::cout << TC_RED   << "[  FAILED  ] " << failed << " test(s)\n" << TC_RESET;
    std::cout << "\n";
    return (failed > 0) ? 1 : 0;
}
