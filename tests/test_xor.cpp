/**
 * @file test_xor.cpp
 * @brief Unit tests for the XOR cipher (EncryptType::XOR).
 *
 * Verifies:
 *  - encrypt then decrypt restores original (self-inverse)
 *  - empty key is a no-op
 *  - empty buffer is safe
 *  - single-byte key
 *  - key longer than data
 *  - key shorter than data (wrapping)
 *  - binary / non-ASCII data
 *  - idempotency: encrypt twice restores original
 */

#include "../../helpers/test_framework.hpp"
#include "../../../crypto.hpp"
#include <vector>
#include <cstdint>

// ── helpers ───────────────────────────────────────────────────────────────────

static std::vector<uint8_t> make_buf(const char* s) {
    return std::vector<uint8_t>(s, s + std::strlen(s));
}

static bool bufs_equal(const std::vector<uint8_t>& a,
                       const std::vector<uint8_t>& b) {
    return a == b;
}

// ── tests ─────────────────────────────────────────────────────────────────────

TEST(XOR, EncryptDecryptRestoresOriginal) {
    std::vector<uint8_t> original = make_buf("Hello, SafeSocket!");
    std::vector<uint8_t> data     = original;

    crypto_encrypt(data, EncryptType::XOR, "secretkey");
    ASSERT_FALSE(bufs_equal(data, original));   // encrypted differs

    crypto_decrypt(data, EncryptType::XOR, "secretkey");
    ASSERT_TRUE(bufs_equal(data, original));    // restored
}

TEST(XOR, EmptyKeyIsNoOp) {
    std::vector<uint8_t> data = make_buf("unchanged");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "");
    ASSERT_TRUE(bufs_equal(data, orig));
}

TEST(XOR, EmptyBufferIsSafe) {
    std::vector<uint8_t> empty;
    // Should not crash
    crypto_encrypt(empty, EncryptType::XOR, "anykey");
    crypto_decrypt(empty, EncryptType::XOR, "anykey");
    ASSERT_EQ((int)empty.size(), 0);
}

TEST(XOR, SingleByteKey) {
    std::vector<uint8_t> data = {0x41, 0x42, 0x43};
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "A");
    crypto_decrypt(data, EncryptType::XOR, "A");
    ASSERT_TRUE(bufs_equal(data, orig));
}

TEST(XOR, KeyLongerThanData) {
    std::vector<uint8_t> data = {0x01, 0x02};
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "verylongkeyindeed");
    crypto_decrypt(data, EncryptType::XOR, "verylongkeyindeed");
    ASSERT_TRUE(bufs_equal(data, orig));
}

TEST(XOR, KeyWrapsAroundData) {
    // 20 bytes of data, 3-byte key — key must wrap 6+ times
    std::vector<uint8_t> data(20, 0xAA);
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "key");
    crypto_decrypt(data, EncryptType::XOR, "key");
    ASSERT_TRUE(bufs_equal(data, orig));
}

TEST(XOR, BinaryData) {
    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; ++i) data[i] = (uint8_t)i;
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "binary\x00\xFF", 8);
    crypto_decrypt(data, EncryptType::XOR, "binary\x00\xFF", 8);
    ASSERT_TRUE(bufs_equal(data, orig));
}

TEST(XOR, SelfInverse_EncryptTwiceRestores) {
    std::vector<uint8_t> data = make_buf("double encrypt");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "k");
    crypto_encrypt(data, EncryptType::XOR, "k");   // second encrypt == decrypt
    ASSERT_TRUE(bufs_equal(data, orig));
}

TEST(XOR, WrongKeyProducesWrongResult) {
    std::vector<uint8_t> data = make_buf("secret message");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::XOR, "rightkey");
    crypto_decrypt(data, EncryptType::XOR, "wrongkey");
    ASSERT_FALSE(bufs_equal(data, orig));
}

int main() { return RUN_ALL_TESTS(); }
