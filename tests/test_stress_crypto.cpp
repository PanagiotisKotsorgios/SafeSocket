/**
 * @file test_stress_crypto.cpp
 * @brief Crypto throughput and correctness under large data volumes.
 *
 * Verifies that all three ciphers:
 *  - correctly round-trip 1 MB payloads
 *  - maintain correct byte boundaries when used repeatedly in sequence
 *  - do not exhibit any silent corruption on large random-looking inputs
 */

#include "../helpers/test_framework.hpp"
#include "../../crypto.hpp"
#include <vector>
#include <cstdint>
#include <chrono>
#include <iostream>

static std::vector<uint8_t> make_random_like(size_t n) {
    // Deterministic "random-looking" data (linear congruential generator)
    std::vector<uint8_t> v(n);
    uint32_t s = 0xDEADBEEF;
    for (size_t i = 0; i < n; ++i) {
        s = s * 1664525u + 1013904223u;
        v[i] = (uint8_t)(s >> 24);
    }
    return v;
}

static double elapsed_ms(std::chrono::steady_clock::time_point t0) {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now() - t0).count() / 1000.0;
}

// ── XOR ───────────────────────────────────────────────────────────────────────

TEST(StressCrypto, XOR_OneMegabyte) {
    const size_t SIZE = 1 << 20;  // 1 MB
    auto data = make_random_like(SIZE);
    auto orig = data;

    auto t0 = std::chrono::steady_clock::now();
    crypto_encrypt(data, EncryptType::XOR, "stresskey");
    crypto_decrypt(data, EncryptType::XOR, "stresskey");
    double ms = elapsed_ms(t0);

    ASSERT_TRUE(data == orig);
    std::cout << "  XOR 1MB round-trip: " << ms << " ms\n";
}

TEST(StressCrypto, XOR_RepeatedCycles) {
    const size_t SIZE = 4096;
    auto data = make_random_like(SIZE);
    auto orig = data;

    for (int i = 0; i < 1000; ++i) {
        crypto_encrypt(data, EncryptType::XOR, "cyclekey");
        crypto_decrypt(data, EncryptType::XOR, "cyclekey");
    }
    ASSERT_TRUE(data == orig);
}

// ── Vigenere ──────────────────────────────────────────────────────────────────

TEST(StressCrypto, Vigenere_OneMegabyte) {
    const size_t SIZE = 1 << 20;
    auto data = make_random_like(SIZE);
    auto orig = data;

    auto t0 = std::chrono::steady_clock::now();
    crypto_encrypt(data, EncryptType::VIGENERE, "longvigenerepassphrase");
    crypto_decrypt(data, EncryptType::VIGENERE, "longvigenerepassphrase");
    double ms = elapsed_ms(t0);

    ASSERT_TRUE(data == orig);
    std::cout << "  Vigenere 1MB round-trip: " << ms << " ms\n";
}

TEST(StressCrypto, Vigenere_WrapAroundAtEveryByteBoundary) {
    // Key of length 1 byte — every byte uses same shift
    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; ++i) data[i] = (uint8_t)i;
    auto orig = data;

    crypto_encrypt(data, EncryptType::VIGENERE, "\x7F");  // shift 127
    crypto_decrypt(data, EncryptType::VIGENERE, "\x7F");
    ASSERT_TRUE(data == orig);
}

// ── RC4 ───────────────────────────────────────────────────────────────────────

TEST(StressCrypto, RC4_OneMegabyte) {
    const size_t SIZE = 1 << 20;
    auto data = make_random_like(SIZE);
    auto orig = data;

    auto t0 = std::chrono::steady_clock::now();
    crypto_encrypt(data, EncryptType::RC4, "rc4stresspassphrase");
    crypto_decrypt(data, EncryptType::RC4, "rc4stresspassphrase");
    double ms = elapsed_ms(t0);

    ASSERT_TRUE(data == orig);
    std::cout << "  RC4 1MB round-trip: " << ms << " ms\n";
}

TEST(StressCrypto, RC4_FourMegabytes) {
    const size_t SIZE = 4 << 20;  // 4 MB
    auto data = make_random_like(SIZE);
    auto orig = data;

    crypto_encrypt(data, EncryptType::RC4, "bigdata");
    crypto_decrypt(data, EncryptType::RC4, "bigdata");
    ASSERT_TRUE(data == orig);
}

TEST(StressCrypto, RC4_RepeatedSmallChunks) {
    // Simulate many small packet encryptions (each gets fresh RC4 state)
    const std::string key = "chunkkey";
    bool all_ok = true;

    for (int i = 0; i < 10000; ++i) {
        std::vector<uint8_t> chunk = {
            (uint8_t)(i & 0xFF),
            (uint8_t)((i >> 8) & 0xFF),
            (uint8_t)(i & 0xAA)
        };
        auto orig = chunk;
        crypto_encrypt(chunk, EncryptType::RC4, key);
        crypto_decrypt(chunk, EncryptType::RC4, key);
        if (chunk != orig) { all_ok = false; break; }
    }
    ASSERT_TRUE(all_ok);
}

// ── All algorithms back-to-back ───────────────────────────────────────────────

TEST(StressCrypto, AllAlgorithmsSequential) {
    auto data = make_random_like(65536);
    auto orig = data;

    crypto_encrypt(data, EncryptType::XOR,      "k1");
    crypto_decrypt(data, EncryptType::XOR,      "k1");
    ASSERT_TRUE(data == orig);

    crypto_encrypt(data, EncryptType::VIGENERE, "k2");
    crypto_decrypt(data, EncryptType::VIGENERE, "k2");
    ASSERT_TRUE(data == orig);

    crypto_encrypt(data, EncryptType::RC4,      "k3");
    crypto_decrypt(data, EncryptType::RC4,      "k3");
    ASSERT_TRUE(data == orig);
}

int main() { return RUN_ALL_TESTS(); }
