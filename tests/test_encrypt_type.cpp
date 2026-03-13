/**
 * @file test_encrypt_type.cpp
 * @brief Unit tests for EncryptType string conversion helpers.
 *
 * Verifies:
 *  - encrypt_type_name() for all four values
 *  - encrypt_type_from_string() for uppercase / lowercase / mixed
 *  - unknown strings fall back to NONE
 *  - round-trip: from_string(name(t)) == t for all t
 *  - NONE mode leaves buffers unchanged
 */

#include "../../helpers/test_framework.hpp"
#include "../../../crypto.hpp"
#include <cstring>

TEST(EncryptType, NamesToStrings) {
    ASSERT_STREQ(encrypt_type_name(EncryptType::NONE),     "NONE");
    ASSERT_STREQ(encrypt_type_name(EncryptType::XOR),      "XOR");
    ASSERT_STREQ(encrypt_type_name(EncryptType::VIGENERE), "VIGENERE");
    ASSERT_STREQ(encrypt_type_name(EncryptType::RC4),      "RC4");
}

TEST(EncryptType, ParseUpperCase) {
    ASSERT_EQ((int)encrypt_type_from_string("NONE"),     (int)EncryptType::NONE);
    ASSERT_EQ((int)encrypt_type_from_string("XOR"),      (int)EncryptType::XOR);
    ASSERT_EQ((int)encrypt_type_from_string("VIGENERE"), (int)EncryptType::VIGENERE);
    ASSERT_EQ((int)encrypt_type_from_string("RC4"),      (int)EncryptType::RC4);
}

TEST(EncryptType, ParseLowerCase) {
    ASSERT_EQ((int)encrypt_type_from_string("none"),     (int)EncryptType::NONE);
    ASSERT_EQ((int)encrypt_type_from_string("xor"),      (int)EncryptType::XOR);
    ASSERT_EQ((int)encrypt_type_from_string("vigenere"), (int)EncryptType::VIGENERE);
    ASSERT_EQ((int)encrypt_type_from_string("rc4"),      (int)EncryptType::RC4);
}

TEST(EncryptType, ParseMixedCase) {
    ASSERT_EQ((int)encrypt_type_from_string("Rc4"),      (int)EncryptType::RC4);
    ASSERT_EQ((int)encrypt_type_from_string("Xor"),      (int)EncryptType::XOR);
    ASSERT_EQ((int)encrypt_type_from_string("Vigenere"), (int)EncryptType::VIGENERE);
}

TEST(EncryptType, UnknownStringFallsBackToNone) {
    ASSERT_EQ((int)encrypt_type_from_string("aes"),     (int)EncryptType::NONE);
    ASSERT_EQ((int)encrypt_type_from_string(""),        (int)EncryptType::NONE);
    ASSERT_EQ((int)encrypt_type_from_string("BLOWFISH"),(int)EncryptType::NONE);
    ASSERT_EQ((int)encrypt_type_from_string("123"),     (int)EncryptType::NONE);
}

TEST(EncryptType, RoundTrip) {
    EncryptType types[] = {
        EncryptType::NONE,
        EncryptType::XOR,
        EncryptType::VIGENERE,
        EncryptType::RC4
    };
    for (auto t : types) {
        ASSERT_EQ((int)encrypt_type_from_string(encrypt_type_name(t)), (int)t);
    }
}

TEST(EncryptType, NoneIsNoOp) {
    std::vector<uint8_t> data = {'H','e','l','l','o'};
    std::vector<uint8_t> orig = data;

    crypto_encrypt(data, EncryptType::NONE, "anykey");
    ASSERT_TRUE(data == orig);

    crypto_decrypt(data, EncryptType::NONE, "anykey");
    ASSERT_TRUE(data == orig);
}

int main() { return RUN_ALL_TESTS(); }
