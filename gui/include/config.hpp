/**
 * @file config.hpp
 * @brief Runtime configuration storage and persistence for SafeSocket.
 *
 * Declares the @ref Config structure that holds every tunable parameter used by
 * both the server and client, as well as the global @ref g_config instance that
 * is shared across all translation units.
 *
 * Configuration values can be sourced from three places, in increasing priority:
 *  1. Compiled-in defaults (member initializers in this struct).
 *  2. A plain-text @c .conf file loaded via @ref Config::load().
 *  3. Command-line arguments parsed in @c main.cpp.
 *
 * Values can also be mutated at runtime through the @c /set CLI command, which
 * ultimately calls @ref Config::set().
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#pragma once

#include <string>
#include <map>
#include "crypto.hpp"

// ============================================================
//  Config Structure
// ============================================================

/**
 * @brief Aggregates all runtime-configurable parameters for SafeSocket.
 *
 * Both the @ref Server and @ref Client read from the single global instance
 * @ref g_config.  Fields are grouped into logical sections that mirror the
 * sections written by @ref Config::save().
 *
 * Thread-safety note: concurrent reads are safe, but writes via @ref Config::set()
 * are not protected by a mutex.  Runtime changes should only be made from the
 * CLI thread.
 */
struct Config {
    // --------------------------------------------------------
    //  [network]
    // --------------------------------------------------------

    std::string host          = "127.0.0.1"; ///< IPv4 address the server binds to, or the server IP the client connects to.
    int         port          = 9000;         ///< TCP port number used for all connections.
    int         max_clients   = 64;           ///< Maximum number of simultaneous client connections (server only).
    int         recv_timeout  = 300;          ///< Socket receive timeout in seconds; 0 disables the timeout entirely.
    int         connect_timeout = 10;         ///< Seconds to wait for a client-side @c connect() to succeed before giving up.

    // --------------------------------------------------------
    //  [identity]
    // --------------------------------------------------------

    std::string nickname      = "anonymous";        ///< Display name sent to the server during the handshake (client only).
    std::string server_name   = "SafeSocket-Server"; ///< Human-readable name of this server instance shown in the startup banner.
    std::string motd          = "Welcome to SafeSocket!"; ///< Message of the Day broadcast to every client upon connection (server only).

    // --------------------------------------------------------
    //  [security]
    // --------------------------------------------------------

    EncryptType encrypt_type  = EncryptType::NONE; ///< Symmetric encryption algorithm applied to packet payloads. See @c EncryptType.
    std::string encrypt_key   = "";                ///< Passphrase used by @c encrypt_type; empty means unkeyed (relevant only for NONE).
    bool        require_key   = false;             ///< When @c true the server demands a matching @c access_key from every client (server only).
    std::string access_key    = "";                ///< Shared secret clients must supply in @ref MSG_CONNECT to be admitted (server only).

    // --------------------------------------------------------
    //  [transfer]
    // --------------------------------------------------------

    std::string download_dir  = "./downloads"; ///< Filesystem path where received files are stored; created automatically if absent.
    bool        auto_accept_files = false;     ///< When @c true the client accepts incoming file transfers without prompting the user.
    size_t      max_file_size = 0;             ///< Maximum accepted incoming file size in bytes; 0 means unlimited.

    // --------------------------------------------------------
    //  [logging]
    // --------------------------------------------------------

    bool        log_to_file   = false;             ///< When @c true all log messages are also written to @c log_file.
    std::string log_file      = "safesocket.log";  ///< Path to the log file used when @c log_to_file is enabled.
    bool        verbose       = false;             ///< When @c true additional diagnostic output is emitted to @c stdout.

    // --------------------------------------------------------
    //  [keepalive]
    // --------------------------------------------------------

    bool        keepalive     = true; ///< Enables the server keepalive thread that sends @ref MSG_PING to all clients periodically.
    int         ping_interval = 30;   ///< Interval in seconds between successive keepalive pings (server only).

    // --------------------------------------------------------
    //  [misc]
    // --------------------------------------------------------

    bool        color_output  = true;  ///< Enables ANSI escape-code colour in terminal output; automatically disabled on Windows if unsupported.
    int         buffer_size   = 4096;  ///< Internal I/O buffer size hint in bytes (informational; not yet used for socket buffer sizes).

    // --------------------------------------------------------
    //  Methods
    // --------------------------------------------------------

    /**
     * @brief Print a formatted summary of all configuration values to @c stdout.
     *
     * Sensitive fields (@c encrypt_key and @c access_key) are masked as
     * @c "***hidden***" when they are non-empty.
     */
    void print() const;

    /**
     * @brief Load configuration from a plain-text key=value file.
     *
     * Lines beginning with @c '#' or @c ';' are treated as comments and skipped.
     * Each line is expected to have the form @c key=value; everything after
     * an inline @c " #" comment marker is stripped.  Unknown keys are silently
     * ignored (the underlying @ref set() call will emit a warning to @c stderr).
     *
     * @param path  Filesystem path of the configuration file to read.
     * @return      @c true if the file was opened and parsed successfully,
     *              @c false if the file could not be opened.
     */
    bool load(const std::string& path);

    /**
     * @brief Persist the current configuration to a plain-text file.
     *
     * Writes all fields including section headers.  Sensitive fields are
     * written in plaintext; protect the file with appropriate OS permissions
     * if access control is required.
     *
     * @param path  Filesystem path of the configuration file to write.
     * @return      @c true if the file was created/truncated and written successfully,
     *              @c false if the file could not be opened for writing.
     */
    bool save(const std::string& path) const;

    /**
     * @brief Set a single configuration field by name, parsing the value string.
     *
     * Key matching is case-insensitive.  Boolean values accept @c "true",
     * @c "1", @c "yes", or @c "on" for @c true; anything else is @c false.
     * Integer values are parsed with @c std::stoi(); a parse failure falls back
     * to the field's current value (or a hardcoded default).
     *
     * @param key    Case-insensitive configuration field name (e.g. @c "port").
     * @param value  String representation of the new value to assign.
     *
     * @note Emits a warning to @c stderr for unrecognised keys.
     */
    void set(const std::string& key, const std::string& value);

    /**
     * @brief Retrieve the current value of a configuration field as a string.
     *
     * The inverse of @ref set().  Sensitive fields (@c encrypt_key and
     * @c access_key) are masked when non-empty.
     *
     * @param key  Case-insensitive configuration field name.
     * @return     String representation of the current value, or
     *             @c "(unknown key)" for unrecognised names.
     */
    std::string get(const std::string& key) const;

    /**
     * @brief Print a help listing of all valid configuration keys to @c stdout.
     *
     * Includes the key name, a brief description, and the default value for
     * each configurable field.  Intended to be shown in response to the
     * @c /confighelp CLI command.
     */
    void print_help() const;
};

// ============================================================
//  Global Instance
// ============================================================

/**
 * @brief Global singleton configuration instance shared across all modules.
 *
 * Defined in @c config.cpp.  Every translation unit that includes this header
 * can read from or write to @c g_config.  The server and client both reference
 * this object directly rather than receiving a copy, so runtime changes made
 * via the CLI are immediately visible everywhere.
 */
extern Config g_config;
