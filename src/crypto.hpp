/**
 * @file crypto.hpp
 * @brief Lightweight symmetric encryption/decryption support for SafeSocket.
 *
 * Provides four selectable algorithms that require no external libraries:
 *  - @c NONE     — plaintext; no transformation applied.
 *  - @c XOR      — repeating-key XOR; fast but weak, suitable for obfuscation only.
 *  - @c VIGENERE — byte-level Vigenère cipher operating across the full 0–255 range.
 *  - @c RC4      — RC4 stream cipher; stronger key-based encryption for practical use.
 *
 * All algorithms operate **in-place** on a @c std::vector<uint8_t> buffer, so no
 * additional heap allocation is required beyond the buffer itself.
 *
 * ### Symmetry
 * XOR and RC4 are self-inverse: calling @ref crypto_encrypt() and then
 * @ref crypto_decrypt() (or vice versa) with the same key and algorithm
 * restores the original data.  Vigenère uses separate encrypt/decrypt
 * paths (addition vs. subtraction modulo 256).
 *
 * ### Usage Example
 * @code
 * std::vector<uint8_t> data = {'H','e','l','l','o'};
 * crypto_encrypt(data, EncryptType::RC4, "mypassword");
 * // ... transmit data ...
 * crypto_decrypt(data, EncryptType::RC4, "mypassword");
 * // data is restored to {'H','e','l','l','o'}
 * @endcode
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

// ============================================================
//  EncryptType Enumeration
// ============================================================

/**
 * @brief Selects the symmetric encryption algorithm to apply to packet payloads.
 *
 * The underlying type is @c int to allow it to be stored in configuration files
 * and transmitted as a simple integer where needed.
 */
enum class EncryptType {
    NONE     = 0,  ///< No encryption; data is transmitted as plaintext.
    XOR      = 1,  ///< Repeating-key XOR; very fast, suitable for lightweight obfuscation only.
    VIGENERE = 2,  ///< Byte-level Vigenère cipher operating across the full 0–255 range.
    RC4      = 3,  ///< RC4 stream cipher; key-scheduled pseudo-random XOR, stronger than XOR/Vigenère.
};

// ============================================================
//  Helper Functions
// ============================================================

/**
 * @brief Return the canonical string name of an @ref EncryptType value.
 *
 * The returned string matches the tokens accepted by @ref encrypt_type_from_string(),
 * so @c encrypt_type_from_string(encrypt_type_name(t)) == t for all valid @p t.
 *
 * @param t  The encryption type to name.
 * @return   One of @c "NONE", @c "XOR", @c "VIGENERE", @c "RC4", or @c "UNKNOWN"
 *           for any value not covered by the enumeration.
 */
std::string encrypt_type_name(EncryptType t);

/**
 * @brief Parse a string into an @ref EncryptType value.
 *
 * Comparison is case-insensitive; @c "rc4", @c "RC4", and @c "Rc4" all map to
 * @c EncryptType::RC4.  Any unrecognised string returns @c EncryptType::NONE
 * rather than throwing an exception, so configuration files with typos fall
 * back to plaintext silently.
 *
 * @param s  The string to parse (e.g. @c "RC4", @c "xor", @c "VIGENERE").
 * @return   The matching @ref EncryptType, or @c EncryptType::NONE if unrecognised.
 */
EncryptType encrypt_type_from_string(const std::string& s);

// ============================================================
//  In-Place Crypto API
// ============================================================

/**
 * @brief Encrypt a byte buffer in-place using the specified algorithm and key.
 *
 * Modifies @p buf directly; no copy is made.  Passing an empty @p key to a
 * keyed algorithm (XOR, Vigenère, RC4) is a no-op for that algorithm, leaving
 * @p buf unchanged.
 *
 * @param[in,out] buf   Buffer of bytes to encrypt; modified in-place.
 * @param[in]     type  Encryption algorithm to apply.
 * @param[in]     key   Passphrase or key material; interpretation depends on @p type.
 *
 * @note For @c EncryptType::NONE this function returns immediately without
 *       touching @p buf.
 */
void crypto_encrypt(std::vector<uint8_t>& buf,
                    EncryptType type,
                    const std::string& key);

/**
 * @brief Decrypt a byte buffer in-place using the specified algorithm and key.
 *
 * Reverses the transformation applied by @ref crypto_encrypt() when called with
 * the same @p type and @p key.  For symmetric algorithms (XOR, RC4) this
 * function is identical to @ref crypto_encrypt(); for Vigenère a distinct
 * subtraction-based path is used.
 *
 * @param[in,out] buf   Buffer of bytes to decrypt; modified in-place.
 * @param[in]     type  Encryption algorithm to reverse.
 * @param[in]     key   Passphrase or key material; must match the value used during encryption.
 *
 * @note For @c EncryptType::NONE this function returns immediately without
 *       touching @p buf.
 */
void crypto_decrypt(std::vector<uint8_t>& buf,
                    EncryptType type,
                    const std::string& key);
