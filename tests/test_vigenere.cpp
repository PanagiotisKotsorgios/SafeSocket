/**
 * @file test_vigenere.cpp
 * @brief Unit tests for the Vigenère cipher (EncryptType::VIGENERE).
 *
 * Verifies:
 *  - encrypt then decrypt restores original
 *  - empty key is a no-op
 *  - empty buffer is safe
 *  - byte wrap-around at 255 → 0 and 0 → 255
 *  - key-length wrapping
 *  - encrypt and decrypt are NOT the same operation (unlike XOR/RC4)
 *  - wrong key does not decrypt correctly
 */

#include "../../helpers/test_framework.hpp"
#include "../../../crypto.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

static std::vector<uint8_t> make_buf(const char* s) {
    return std::vector<uint8_t>(s, s + std::strlen(s));
}

TEST(Vigenere, EncryptDecryptRestoresOriginal) {
    std::vector<uint8_t> original = make_buf("Hello SafeSocket Vigenere!");
    std::vector<uint8_t> data = original;

    crypto_encrypt(data, EncryptType::VIGENERE, "mykey");
    ASSERT_FALSE(data == original);

    crypto_decrypt(data, EncryptType::VIGENERE, "mykey");
    ASSERT_TRUE(data == original);
}

TEST(Vigenere, EmptyKeyIsNoOp) {
    std::vector<uint8_t> data = make_buf("no change");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, "");
    ASSERT_TRUE(data == orig);

    crypto_decrypt(data, EncryptType::VIGENERE, "");
    ASSERT_TRUE(data == orig);
}

TEST(Vigenere, EmptyBufferIsSafe) {
    std::vector<uint8_t> empty;
    crypto_encrypt(empty, EncryptType::VIGENERE, "key");
    crypto_decrypt(empty, EncryptType::VIGENERE, "key");
    ASSERT_EQ((int)empty.size(), 0);
}

TEST(Vigenere, ByteWrapAroundAtMax) {
    // 0xFF + 1 should wrap to 0x00
    std::vector<uint8_t> data = {0xFF};
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, "\x01");  // shift +1
    ASSERT_EQ((int)data[0], 0x00);

    crypto_decrypt(data, EncryptType::VIGENERE, "\x01");
    ASSERT_TRUE(data == orig);
}

TEST(Vigenere, ByteWrapAroundAtMin) {
    // 0x00 - 1 should wrap to 0xFF
    std::vector<uint8_t> data = {0x00};
    std::vector<uint8_t> orig = data;

    crypto_decrypt(data, EncryptType::VIGENERE, "\x01");
    ASSERT_EQ((int)data[0], 0xFF);

    crypto_encrypt(data, EncryptType::VIGENERE, "\x01");
    ASSERT_TRUE(data == orig);
}

TEST(Vigenere, KeyWrapsAcrossData) {
    std::vector<uint8_t> data(50, 0x7F);
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, "ab");  // 2-byte key, 50 bytes data
    crypto_decrypt(data, EncryptType::VIGENERE, "ab");
    ASSERT_TRUE(data == orig);
}

TEST(Vigenere, EncryptNotSameAsDecrypt) {
    // Vigenere encrypt and decrypt are DIFFERENT operations
    std::vector<uint8_t> data1 = make_buf("test");
    std::vector<uint8_t> data2 = make_buf("test");

    crypto_encrypt(data1, EncryptType::VIGENERE, "key");
    crypto_decrypt(data2, EncryptType::VIGENERE, "key");

    ASSERT_FALSE(data1 == data2);   // they should differ (unlike XOR)
}

TEST(Vigenere, FullByteRange) {
    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; ++i) data[i] = (uint8_t)i;
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, "fullrange");
    crypto_decrypt(data, EncryptType::VIGENERE, "fullrange");
    ASSERT_TRUE(data == orig);
}

TEST(Vigenere, WrongKeyFails) {
    std::vector<uint8_t> data = make_buf("sensitive");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, "correct");
    crypto_decrypt(data, EncryptType::VIGENERE, "incorrect");
    ASSERT_FALSE(data == orig);
}

TEST(Vigenere, ZeroShiftKeyIsNoOp) {
    // A key of \x00 bytes shifts by 0 — data should be unchanged
    std::vector<uint8_t> data = make_buf("zero shift");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, std::string(5, '\x00'));
    ASSERT_TRUE(data == orig);
}

int main() { return RUN_ALL_TESTS(); }
