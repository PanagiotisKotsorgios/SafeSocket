/**
 * @file crypto.cpp
 * @brief Implementation of SafeSocket's built-in symmetric encryption algorithms.
 *
 * Provides concrete implementations for the four algorithms declared in
 * @ref crypto.hpp.  All algorithms are implemented from scratch with no
 * dependency on OpenSSL or any other external cryptographic library.
 *
 * ### Algorithm Notes
 *
 * **XOR** (`xor_crypt`)
 * Simple repeating-key XOR.  The key byte at position @c i%klen is XOR-ed with
 * the plaintext byte at position @c i.  The operation is its own inverse, so the
 * same function is used for both encryption and decryption.
 * Security: trivially broken by known-plaintext; suitable only for basic
 * obfuscation.
 *
 * **Vigenere** (`vigenere_encrypt` / `vigenere_decrypt`)
 * Byte-level Vigenère operating on the full 0–255 range (unlike the classic
 * text-only variant).  Encryption adds the key byte modulo 256; decryption
 * subtracts it modulo 256.  Stronger than XOR against simple frequency
 * analysis, but still vulnerable to the Kasiski examination.
 *
 * **RC4** (`RC4State`, `rc4_crypt`)
 * Standard RC4 (also known as ARC4) stream cipher.  The key-scheduling
 * algorithm (KSA) initialises the 256-byte permutation array, and the
 * pseudo-random generation algorithm (PRGA) produces a keystream that is
 * XOR-ed with the data.  RC4 is symmetric: the same PRGA sequence is
 * generated from the same key, so the same function encrypts and decrypts.
 * Security: RC4 has known weaknesses (biased output bytes, susceptibility to
 * related-key attacks) and should not be used in new security-critical systems,
 * but is significantly stronger than XOR or Vigenère for casual use.
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#include "crypto.hpp"
#include <algorithm>
#include <stdexcept>

// ============================================================
//  Name ? Enum Conversion
// ============================================================

/**
 * @brief Convert an @ref EncryptType value to its canonical string name.
 *
 * The returned string is uppercase and matches the tokens that
 * @ref encrypt_type_from_string() accepts.
 *
 * @param t  Encryption type to convert.
 * @return   @c "NONE", @c "XOR", @c "VIGENERE", @c "RC4", or @c "UNKNOWN"
 *           for any value not covered by the enumeration.
 */
std::string encrypt_type_name(EncryptType t) {
    switch(t) {
        case EncryptType::NONE:     return "NONE";
        case EncryptType::XOR:      return "XOR";
        case EncryptType::VIGENERE: return "VIGENERE";
        case EncryptType::RC4:      return "RC4";
    }
    return "UNKNOWN";
}

/**
 * @brief Parse a string into an @ref EncryptType value (case-insensitive).
 *
 * The input is uppercased before comparison so that @c "rc4", @c "RC4", and
 * @c "Rc4" all resolve to @c EncryptType::RC4.  Unrecognised strings fall back
 * to @c EncryptType::NONE without throwing.
 *
 * @param s  String to parse.
 * @return   The matching @ref EncryptType, or @c EncryptType::NONE if unknown.
 */
EncryptType encrypt_type_from_string(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    if (u == "XOR")      return EncryptType::XOR;
    if (u == "VIGENERE") return EncryptType::VIGENERE;
    if (u == "RC4")      return EncryptType::RC4;
    return EncryptType::NONE;
}

// ============================================================
//  XOR Cipher
// ============================================================

/**
 * @brief Apply repeating-key XOR to a byte buffer in-place.
 *
 * Each byte at index @c i is XOR-ed with @c key[i % klen], where @c klen is
 * the length of the key.  The operation is its own inverse, so this function
 * serves as both encrypt and decrypt.
 *
 * If @p key is empty the function returns immediately, leaving @p buf unchanged.
 *
 * @param[in,out] buf  Buffer to transform in-place.
 * @param[in]     key  Repeating XOR key; must not be empty for any transformation
 *                     to occur.
 */
static void xor_crypt(std::vector<uint8_t>& buf, const std::string& key) {
    if (key.empty()) return;
    size_t klen = key.size();
    for (size_t i = 0; i < buf.size(); ++i)
        buf[i] ^= static_cast<uint8_t>(key[i % klen]);
}

// ============================================================
//  Vigenère Cipher (byte-level, 0–255 range)
// ============================================================

/**
 * @brief Encrypt a byte buffer using a byte-level Vigenère cipher.
 *
 * Each byte at index @c i is replaced by @c (buf[i] + shift) & 0xFF, where
 * @c shift is the unsigned value of @c key[i % klen].  This extends the
 * classic Vigenère cipher from the 26-letter alphabet to the full 8-bit range.
 *
 * If @p key is empty the function returns immediately, leaving @p buf unchanged.
 *
 * @param[in,out] buf  Buffer to encrypt in-place.
 * @param[in]     key  Vigenère key; each byte contributes a shift value in [0, 255].
 */
static void vigenere_encrypt(std::vector<uint8_t>& buf, const std::string& key) {
    if (key.empty()) return;
    size_t klen = key.size();
    for (size_t i = 0; i < buf.size(); ++i) {
        int shift = static_cast<uint8_t>(key[i % klen]);
        buf[i] = static_cast<uint8_t>((buf[i] + shift) & 0xFF);
    }
}

/**
 * @brief Decrypt a byte buffer that was encrypted with @ref vigenere_encrypt().
 *
 * Reverses the additive shift: each byte at index @c i is replaced by
 * @c (buf[i] - shift + 256) & 0xFF, where @c shift is the unsigned value of
 * @c key[i % klen].  The @c +256 ensures the result stays non-negative before
 * masking to 8 bits.
 *
 * If @p key is empty the function returns immediately, leaving @p buf unchanged.
 *
 * @param[in,out] buf  Buffer to decrypt in-place.
 * @param[in]     key  Vigenère key; must match the key used during encryption.
 */
static void vigenere_decrypt(std::vector<uint8_t>& buf, const std::string& key) {
    if (key.empty()) return;
    size_t klen = key.size();
    for (size_t i = 0; i < buf.size(); ++i) {
        int shift = static_cast<uint8_t>(key[i % klen]);
        buf[i] = static_cast<uint8_t>((buf[i] - shift + 256) & 0xFF);
    }
}

// ============================================================
//  RC4 Stream Cipher
// ============================================================

/**
 * @brief Internal state for the RC4 (ARC4) stream cipher.
 *
 * Encapsulates both the Key-Scheduling Algorithm (KSA) performed during
 * construction and the Pseudo-Random Generation Algorithm (PRGA) exposed via
 * @ref next().
 *
 * A new @ref RC4State must be created for each message; reusing state across
 * messages would produce the same keystream offset and break confidentiality.
 *
 * @note This structure is an implementation detail; it is not part of the
 *       public API and is defined in an anonymous namespace in the .cpp file.
 */
struct RC4State {
    uint8_t S[256]; ///< The 256-byte permutation array, initialised by the KSA.
    uint8_t i;      ///< PRGA outer index, advanced by one on each call to @ref next().
    uint8_t j;      ///< PRGA inner index, mixed with @c S[i] on each call to @ref next().

    /**
     * @brief Construct an RC4 state from a passphrase using the Key-Scheduling Algorithm.
     *
     * Fills @c S with the identity permutation @c {0, 1, …, 255}, then performs
     * 256 swap operations driven by the key bytes to produce the initial scrambled
     * state.  Key bytes are cycled using modular indexing, so keys shorter than
     * 256 bytes are repeated as needed.
     *
     * @param key  Passphrase to derive the keystream from; should be non-empty.
     */
    RC4State(const std::string& key) : i(0), j(0) {
        /* Initialise S as the identity permutation. */
        for (int k = 0; k < 256; ++k) S[k] = static_cast<uint8_t>(k);
        uint8_t jj = 0;
        size_t klen = key.size();
        /* KSA: scramble S using the key. */
        for (int k = 0; k < 256; ++k) {
            jj = (jj + S[k] + static_cast<uint8_t>(key[k % klen])) & 0xFF;
            std::swap(S[k], S[jj]);
        }
    }

    /**
     * @brief Generate the next byte of the RC4 keystream (PRGA step).
     *
     * Advances @c i by one, mixes @c j with @c S[i], swaps @c S[i] and @c S[j],
     * and returns @c S[(S[i] + S[j]) & 0xFF] as the output keystream byte.
     *
     * @return The next pseudo-random keystream byte in [0, 255].
     */
    uint8_t next() {
        i = (i + 1) & 0xFF;
        j = (j + S[i]) & 0xFF;
        std::swap(S[i], S[j]);
        return S[(S[i] + S[j]) & 0xFF];
    }
};

/**
 * @brief Encrypt or decrypt a byte buffer in-place using the RC4 stream cipher.
 *
 * Creates a fresh @ref RC4State from @p key (running the KSA), then XOR-es each
 * byte of @p buf with the next keystream byte produced by the PRGA.  Because
 * RC4 is a symmetric stream cipher, calling this function a second time with
 * the same key on the ciphertext restores the original plaintext.
 *
 * If @p key is empty the function returns immediately without modifying @p buf.
 *
 * @param[in,out] buf  Buffer to encrypt/decrypt in-place.
 * @param[in]     key  RC4 key (passphrase); longer keys generally improve security.
 */
static void rc4_crypt(std::vector<uint8_t>& buf, const std::string& key) {
    if (key.empty()) return;
    RC4State rc4(key);
    for (auto& byte : buf)
        byte ^= rc4.next();
}

// ============================================================
//  Public API
// ============================================================

/**
 * @brief Encrypt a byte buffer in-place using the specified algorithm.
 *
 * Dispatches to the appropriate internal cipher function based on @p type.
 * For @c EncryptType::NONE the buffer is left completely unchanged.
 *
 * @param[in,out] buf   Buffer to encrypt; modified in-place.
 * @param[in]     type  Algorithm to apply.
 * @param[in]     key   Key/passphrase; an empty key is a no-op for keyed algorithms.
 */
void crypto_encrypt(std::vector<uint8_t>& buf,
                    EncryptType type,
                    const std::string& key)
{
    switch(type) {
        case EncryptType::NONE:     break;
        case EncryptType::XOR:      xor_crypt(buf, key);        break;
        case EncryptType::VIGENERE: vigenere_encrypt(buf, key); break;
        case EncryptType::RC4:      rc4_crypt(buf, key);        break;
    }
}

/**
 * @brief Decrypt a byte buffer in-place using the specified algorithm.
 *
 * Reverses the transformation applied by @ref crypto_encrypt() when called with
 * the same @p type and @p key.
 *
 * For symmetric algorithms (XOR, RC4) the same internal function is called as
 * during encryption — the XOR with an identical keystream restores the original
 * data.  For Vigenère the dedicated subtraction-based @ref vigenere_decrypt()
 * is used.  For @c EncryptType::NONE the buffer is left completely unchanged.
 *
 * @param[in,out] buf   Buffer to decrypt; modified in-place.
 * @param[in]     type  Algorithm to reverse; must match the value used for encryption.
 * @param[in]     key   Key/passphrase; must match the value used for encryption.
 */
void crypto_decrypt(std::vector<uint8_t>& buf,
                    EncryptType type,
                    const std::string& key)
{
    switch(type) {
        case EncryptType::NONE:     break;
        case EncryptType::XOR:      xor_crypt(buf, key);        break;  ///< XOR is self-inverse.
        case EncryptType::VIGENERE: vigenere_decrypt(buf, key); break;
        case EncryptType::RC4:      rc4_crypt(buf, key);        break;  ///< RC4 is self-inverse.
    }
}
