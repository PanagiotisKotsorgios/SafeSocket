/**
 * @file config.cpp
 * @brief Implementation of configuration load, save, set, get, and print for SafeSocket.
 *
 * Defines the global @ref g_config instance and provides all methods declared
 * in @ref config.hpp.  Configuration can be persisted to and loaded from a
 * plain-text INI-style file, and individual keys can be mutated at runtime
 * via @ref Config::set().
 *
 * ### File Format
 * The configuration file uses a simple @c key = value syntax:
 * @code
 * # This is a comment
 * port = 9000
 * encrypt_type = RC4
 * encrypt_key  = mysecret  # inline comment
 * @endcode
 * Lines beginning with @c '#' or @c ';' are comments.  Inline comments after a
 * space-hash (@c " #") sequence are stripped from values.  Section headers
 * (@c [network] etc.) are written by @ref Config::save() for human readability
 * but are ignored by the parser — only @c key=value lines matter.
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#include "config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

/** @brief The single global configuration instance shared across all modules. */
Config g_config;

// ============================================================
//  Private Helper Functions
// ============================================================

/**
 * @brief Strip leading and trailing whitespace from a string.
 *
 * @param s  The string to trim.
 * @return   A copy of @p s with all leading and trailing @c ' ', @c '\\t',
 *           @c '\\r', and @c '\\n' characters removed.
 */
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

/**
 * @brief Parse a string as a decimal integer.
 *
 * Uses @c std::stoi() internally.  Returns @p def if parsing fails for any
 * reason (empty string, non-numeric characters, out-of-range value).
 *
 * @param s    The string to parse.
 * @param def  Fallback value returned on parse failure.
 * @return     The parsed integer value, or @p def on failure.
 */
static int parse_int(const std::string& s, int def = 0) {
    try { return std::stoi(s); }
    catch(...) { return def; }
}

/**
 * @brief Interpret a string as a boolean value.
 *
 * The following values (case-insensitive) are considered @c true:
 * @c "true", @c "1", @c "yes", @c "on".
 * All other values are considered @c false.
 *
 * @param s  The string to interpret.
 * @return   @c true or @c false according to the rules above.
 */
static bool parse_bool(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(), ::tolower);
    return (u == "true" || u == "1" || u == "yes" || u == "on");
}

// ============================================================
//  Config::set
// ============================================================

/**
 * @brief Set a configuration field by name.
 *
 * The key lookup is case-insensitive.  Each recognised key maps directly to
 * a member field; the string value is converted to the appropriate type using
 * @ref parse_int(), @ref parse_bool(), or @ref encrypt_type_from_string() as
 * needed.
 *
 * If @p key does not match any known field a warning is emitted to @c stderr
 * and no change is made.
 *
 * @param key    Case-insensitive name of the configuration field to update.
 * @param value  String representation of the new value.
 */
void Config::set(const std::string& key, const std::string& value) {
    /* Normalise to lowercase for case-insensitive matching. */
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    if (k == "host")              host = value;
    else if (k == "port")         port = parse_int(value, 9000);
    else if (k == "max_clients")  max_clients = parse_int(value, 64);
    else if (k == "recv_timeout") recv_timeout = parse_int(value, 300);
    else if (k == "connect_timeout") connect_timeout = parse_int(value, 10);
    else if (k == "nickname")     nickname = value;
    else if (k == "server_name")  server_name = value;
    else if (k == "motd")         motd = value;
    else if (k == "encrypt_type") encrypt_type = encrypt_type_from_string(value);
    else if (k == "encrypt_key")  encrypt_key = value;
    else if (k == "require_key")  require_key = parse_bool(value);
    else if (k == "access_key")   access_key = value;
    else if (k == "download_dir") download_dir = value;
    else if (k == "auto_accept_files") auto_accept_files = parse_bool(value);
    else if (k == "max_file_size")max_file_size = (size_t)parse_int(value, 0);
    else if (k == "log_to_file")  log_to_file = parse_bool(value);
    else if (k == "log_file")     log_file = value;
    else if (k == "verbose")      verbose = parse_bool(value);
    else if (k == "keepalive")    keepalive = parse_bool(value);
    else if (k == "ping_interval")ping_interval = parse_int(value, 30);
    else if (k == "color_output") color_output = parse_bool(value);
    else if (k == "buffer_size")  buffer_size = parse_int(value, 4096);
    else {
        std::cerr << "[config] Unknown key: " << key << "\n";
    }
}

// ============================================================
//  Config::get
// ============================================================

/**
 * @brief Retrieve the current value of a configuration field as a string.
 *
 * The key lookup is case-insensitive.  Sensitive fields (@c encrypt_key and
 * @c access_key) are returned as @c "***hidden***" when they contain a
 * non-empty value to avoid accidental exposure in logs or console output.
 *
 * @param key  Case-insensitive name of the configuration field to read.
 * @return     Current value as a string, or @c "(unknown key)" for unrecognised names.
 */
std::string Config::get(const std::string& key) const {
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    if (k == "host")              return host;
    if (k == "port")              return std::to_string(port);
    if (k == "max_clients")       return std::to_string(max_clients);
    if (k == "recv_timeout")      return std::to_string(recv_timeout);
    if (k == "connect_timeout")   return std::to_string(connect_timeout);
    if (k == "nickname")          return nickname;
    if (k == "server_name")       return server_name;
    if (k == "motd")              return motd;
    if (k == "encrypt_type")      return encrypt_type_name(encrypt_type);
    if (k == "encrypt_key")       return encrypt_key.empty() ? "(not set)" : "***hidden***";
    if (k == "require_key")       return require_key ? "true" : "false";
    if (k == "access_key")        return access_key.empty() ? "(not set)" : "***hidden***";
    if (k == "download_dir")      return download_dir;
    if (k == "auto_accept_files") return auto_accept_files ? "true" : "false";
    if (k == "max_file_size")     return std::to_string(max_file_size);
    if (k == "log_to_file")       return log_to_file ? "true" : "false";
    if (k == "log_file")          return log_file;
    if (k == "verbose")           return verbose ? "true" : "false";
    if (k == "keepalive")         return keepalive ? "true" : "false";
    if (k == "ping_interval")     return std::to_string(ping_interval);
    if (k == "color_output")      return color_output ? "true" : "false";
    if (k == "buffer_size")       return std::to_string(buffer_size);
    return "(unknown key)";
}

// ============================================================
//  Config::load
// ============================================================

/**
 * @brief Parse a configuration file and apply the contained key-value pairs.
 *
 * The parser handles:
 *  - Comment lines (starting with @c '#' or @c ';') — skipped.
 *  - Blank lines — skipped.
 *  - @c key=value lines — the key and value are trimmed, then passed to
 *    @ref Config::set().
 *  - Inline comments after @c " #" in the value portion — stripped.
 *  - Section headers (e.g. @c [network]) — silently ignored.
 *
 * Unknown keys produce a warning via @ref Config::set() but do not abort
 * the load.
 *
 * @param path  Filesystem path of the configuration file to read.
 * @return      @c true if the file was opened and processed; @c false if the
 *              file could not be opened.
 */
bool Config::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        /* Skip comments and blank lines. */
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;   /* No '=' ? not a key=value line. */
        std::string k = trim(line.substr(0, eq));
        std::string v = trim(line.substr(eq + 1));
        /* Strip inline comments (space-hash sequence). */
        size_t hash = v.find(" #");
        if (hash != std::string::npos) v = trim(v.substr(0, hash));
        set(k, v);
    }
    return true;
}

// ============================================================
//  Config::save
// ============================================================

/**
 * @brief Write the current configuration to a plain-text file.
 *
 * The file is organised into labelled sections for human readability.
 * Sensitive values (@c encrypt_key and @c access_key) are written in plaintext;
 * callers are responsible for applying appropriate file-system permissions.
 *
 * An existing file at @p path is truncated and overwritten.
 *
 * @param path  Filesystem path of the output file.
 * @return      @c true if the file was created and written successfully;
 *              @c false if the file could not be opened.
 */
bool Config::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "# SafeSocket Configuration File\n"
      << "# Generated automatically. Lines starting with # are comments.\n\n"
      << "[network]\n"
      << "host             = " << host << "\n"
      << "port             = " << port << "\n"
      << "max_clients      = " << max_clients << "\n"
      << "recv_timeout     = " << recv_timeout << "\n"
      << "connect_timeout  = " << connect_timeout << "\n\n"
      << "[identity]\n"
      << "nickname         = " << nickname << "\n"
      << "server_name      = " << server_name << "\n"
      << "motd             = " << motd << "\n\n"
      << "[security]\n"
      << "encrypt_type     = " << encrypt_type_name(encrypt_type) << "\n"
      << "encrypt_key      = " << encrypt_key << "\n"
      << "require_key      = " << (require_key ? "true" : "false") << "\n"
      << "access_key       = " << access_key << "\n\n"
      << "[transfer]\n"
      << "download_dir     = " << download_dir << "\n"
      << "auto_accept_files= " << (auto_accept_files ? "true" : "false") << "\n"
      << "max_file_size    = " << max_file_size << "\n\n"
      << "[logging]\n"
      << "log_to_file      = " << (log_to_file ? "true" : "false") << "\n"
      << "log_file         = " << log_file << "\n"
      << "verbose          = " << (verbose ? "true" : "false") << "\n\n"
      << "[keepalive]\n"
      << "keepalive        = " << (keepalive ? "true" : "false") << "\n"
      << "ping_interval    = " << ping_interval << "\n\n"
      << "[misc]\n"
      << "color_output     = " << (color_output ? "true" : "false") << "\n"
      << "buffer_size      = " << buffer_size << "\n";

    return true;
}

// ============================================================
//  Config::print
// ============================================================

/**
 * @brief Print a formatted summary of the current configuration to @c stdout.
 *
 * All fields are displayed, grouped into sections matching those in the config
 * file.  Sensitive values are masked with @c "***hidden***" unless empty,
 * in which case @c "(not set)" is shown.  Numeric timeouts are annotated with
 * units (e.g. @c "300s") and @c max_file_size shows @c "unlimited" for zero.
 */
void Config::print() const {
    std::cout << "=== SafeSocket Configuration ===\n"
              << "  [network]\n"
              << "    host             = " << host << "\n"
              << "    port             = " << port << "\n"
              << "    max_clients      = " << max_clients << "\n"
              << "    recv_timeout     = " << recv_timeout << "s\n"
              << "    connect_timeout  = " << connect_timeout << "s\n"
              << "  [identity]\n"
              << "    nickname         = " << nickname << "\n"
              << "    server_name      = " << server_name << "\n"
              << "    motd             = " << motd << "\n"
              << "  [security]\n"
              << "    encrypt_type     = " << encrypt_type_name(encrypt_type) << "\n"
              << "    encrypt_key      = " << (encrypt_key.empty() ? "(not set)" : "***hidden***") << "\n"
              << "    require_key      = " << (require_key ? "yes" : "no") << "\n"
              << "    access_key       = " << (access_key.empty() ? "(not set)" : "***hidden***") << "\n"
              << "  [transfer]\n"
              << "    download_dir     = " << download_dir << "\n"
              << "    auto_accept_files= " << (auto_accept_files ? "yes" : "no") << "\n"
              << "    max_file_size    = " << (max_file_size == 0 ? "unlimited" : std::to_string(max_file_size) + " bytes") << "\n"
              << "  [logging]\n"
              << "    log_to_file      = " << (log_to_file ? "yes" : "no") << "\n"
              << "    log_file         = " << log_file << "\n"
              << "    verbose          = " << (verbose ? "yes" : "no") << "\n"
              << "  [keepalive]\n"
              << "    keepalive        = " << (keepalive ? "yes" : "no") << "\n"
              << "    ping_interval    = " << ping_interval << "s\n"
              << "  [misc]\n"
              << "    color_output     = " << (color_output ? "yes" : "no") << "\n"
              << "    buffer_size      = " << buffer_size << " bytes\n"
              << "================================\n";
}

// ============================================================
//  Config::print_help
// ============================================================

/**
 * @brief Print a reference listing of all valid configuration keys to @c stdout.
 *
 * Shows each key's name, a brief description of its purpose, and its default
 * value.  Intended to be displayed in response to the @c /confighelp CLI command
 * so that operators can discover valid keys without consulting source code.
 */
void Config::print_help() const {
    std::cout <<
        "Available configuration keys (use /set <key> <value> or edit safesocket.conf):\n"
        "  host             - Server IP to bind/connect (default: 127.0.0.1)\n"
        "  port             - Port number (default: 9000)\n"
        "  max_clients      - Max simultaneous clients (server only, default: 64)\n"
        "  recv_timeout     - Socket recv timeout in seconds (0=infinite, default: 300)\n"
        "  connect_timeout  - Connection timeout in seconds (default: 10)\n"
        "  nickname         - Your display name (default: anonymous)\n"
        "  server_name      - Server display name (server only)\n"
        "  motd             - Message of the Day shown on connect (server only)\n"
        "  encrypt_type     - Encryption: NONE | XOR | VIGENERE | RC4 (default: NONE)\n"
        "  encrypt_key      - Encryption key/passphrase\n"
        "  require_key      - Require access_key to join (true/false, server only)\n"
        "  access_key       - Password clients must supply to connect\n"
        "  download_dir     - Where received files are saved (default: ./downloads)\n"
        "  auto_accept_files- Auto-accept incoming files (true/false, default: false)\n"
        "  max_file_size    - Max file size in bytes (0=unlimited)\n"
        "  log_to_file      - Log output to file (true/false, default: false)\n"
        "  log_file         - Log filename (default: safesocket.log)\n"
        "  verbose          - Show verbose debug output (true/false)\n"
        "  keepalive        - Enable ping/pong keepalive (true/false, default: true)\n"
        "  ping_interval    - Ping interval in seconds (default: 30)\n"
        "  color_output     - Colorized terminal output (true/false, default: true)\n"
        "  buffer_size      - Internal I/O buffer size bytes (default: 4096)\n";
}
