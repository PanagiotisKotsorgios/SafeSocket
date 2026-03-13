/**
 * @file test_fmt_bytes.cpp
 * @brief Unit tests for fmt_bytes() formatting utility.
 *
 * Verifies correct output for B, KB, MB, GB, TB ranges.
 */

#include "../../helpers/test_framework.hpp"
#include "../../../network.hpp"
#include <string>

TEST(FmtBytes, Zero) {
    ASSERT_STREQ(fmt_bytes(0), "0 B");
}

TEST(FmtBytes, OneB) {
    ASSERT_STREQ(fmt_bytes(1), "1 B");
}

TEST(FmtBytes, MaxB) {
    // 1023 bytes — still in B range
    ASSERT_STREQ(fmt_bytes(1023), "1023 B");
}

TEST(FmtBytes, ExactlyOneKB) {
    std::string s = fmt_bytes(1024);
    ASSERT_STREQ(s, "1.00 KB");
}

TEST(FmtBytes, OneAndHalfKB) {
    std::string s = fmt_bytes(1536);  // 1.5 * 1024
    ASSERT_STREQ(s, "1.50 KB");
}

TEST(FmtBytes, ExactlyOneMB) {
    std::string s = fmt_bytes(1024ULL * 1024);
    ASSERT_STREQ(s, "1.00 MB");
}

TEST(FmtBytes, TwoMB) {
    std::string s = fmt_bytes(2ULL * 1024 * 1024);
    ASSERT_STREQ(s, "2.00 MB");
}

TEST(FmtBytes, ExactlyOneGB) {
    std::string s = fmt_bytes(1024ULL * 1024 * 1024);
    ASSERT_STREQ(s, "1.00 GB");
}

TEST(FmtBytes, ExactlyOneTB) {
    std::string s = fmt_bytes(1024ULL * 1024 * 1024 * 1024);
    ASSERT_STREQ(s, "1.00 TB");
}

TEST(FmtBytes, NeverEmpty) {
    uint64_t values[] = {0, 1, 1023, 1024, 1025,
                         1024*1024, 1024ULL*1024*1024,
                         1024ULL*1024*1024*1024};
    for (auto v : values) {
        ASSERT_FALSE(fmt_bytes(v).empty());
    }
}

int main() { return RUN_ALL_TESTS(); }
