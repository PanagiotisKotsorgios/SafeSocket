/**
 * @file test_rc4.cpp
 * @brief Unit tests for the RC4 stream cipher (EncryptType::RC4).
 *
 * Verifies:
 *  - encrypt then decrypt restores original (self-inverse)
 *  - empty key is a no-op
 *  - empty buffer is safe
 *  - same key, two independent encryptions produce same ciphertext
 *  - different keys produce different ciphertext
 *  - wrong key does not decrypt correctly
 *  - large buffer (>256 bytes to cover PRGA wrap)
 *  - known KSA/PRGA test vector (first 3 keystream bytes for key "Key")
 */

#include "../../helpers/test_framework.hpp"
#include "../../../crypto.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

static std::vector<uint8_t> make_buf(const char* s) {
    return std::vector<uint8_t>(s, s + std::strlen(s));
}

TEST(RC4, EncryptDecryptRestoresOriginal) {
    std::vector<uint8_t> original = make_buf("RC4 encryption test payload");
    std::vector<uint8_t> data = original;

    crypto_encrypt(data, EncryptType::RC4, "passphrase");
    ASSERT_FALSE(data == original);

    crypto_decrypt(data, EncryptType::RC4, "passphrase");
    ASSERT_TRUE(data == original);
}

TEST(RC4, EmptyKeyIsNoOp) {
    std::vector<uint8_t> data = make_buf("unchanged");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::RC4, "");
    ASSERT_TRUE(data == orig);
}

TEST(RC4, EmptyBufferIsSafe) {
    std::vector<uint8_t> empty;
    crypto_encrypt(empty, EncryptType::RC4, "key");
    ASSERT_EQ((int)empty.size(), 0);
}

TEST(RC4, DeterministicForSameKey) {
    std::vector<uint8_t> data1 = make_buf("deterministic");
    std::vector<uint8_t> data2 = make_buf("deterministic");

    crypto_encrypt(data1, EncryptType::RC4, "mykey");
    crypto_encrypt(data2, EncryptType::RC4, "mykey");

    ASSERT_TRUE(data1 == data2);
}

TEST(RC4, DifferentKeysProduceDifferentCiphertext) {
    std::vector<uint8_t> data1 = make_buf("same plaintext");
    std::vector<uint8_t> data2 = make_buf("same plaintext");

    crypto_encrypt(data1, EncryptType::RC4, "key1");
    crypto_encrypt(data2, EncryptType::RC4, "key2");

    ASSERT_FALSE(data1 == data2);
}

TEST(RC4, WrongKeyFails) {
    std::vector<uint8_t> data = make_buf("confidential");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::RC4, "rightkey");
    crypto_decrypt(data, EncryptType::RC4, "wrongkey");
    ASSERT_FALSE(data == orig);
}

TEST(RC4, LargeBuffer) {
    // 1024 bytes — exercises PRGA cycling well past the 256-byte S-box
    std::vector<uint8_t> data(1024);
    for (size_t i = 0; i < data.size(); ++i) data[i] = (uint8_t)(i & 0xFF);
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::RC4, "largebufferkey");
    ASSERT_FALSE(data == orig);

    crypto_decrypt(data, EncryptType::RC4, "largebufferkey");
    ASSERT_TRUE(data == orig);
}

TEST(RC4, FullByteRange) {
    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; ++i) data[i] = (uint8_t)i;
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::RC4, "fullrange");
    crypto_decrypt(data, EncryptType::RC4, "fullrange");
    ASSERT_TRUE(data == orig);
}

TEST(RC4, KnownTestVector) {
    // RFC 6229 / Wikipedia: key="Key" (3 bytes), plaintext=zeros
    // Expected first 3 keystream bytes: 0xEB, 0x9F, 0x77
    std::vector<uint8_t> data = {0x00, 0x00, 0x00};
    crypto_encrypt(data, EncryptType::RC4, "Key");
    ASSERT_EQ((int)data[0], 0xEB);
    ASSERT_EQ((int)data[1], 0x9F);
    ASSERT_EQ((int)data[2], 0x77);
}

TEST(RC4, SelfInverse) {
    // Encrypting twice with same key should restore original
    std::vector<uint8_t> data = make_buf("self inverse");
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::RC4, "k");
    crypto_encrypt(data, EncryptType::RC4, "k");
    ASSERT_TRUE(data == orig);
}

int main() { return RUN_ALL_TESTS(); }
