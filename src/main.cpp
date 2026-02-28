/**
 * @file main.cpp
 * @brief Application entry point for SafeSocket.
 *
 * Parses command-line arguments, loads configuration, initialises networking,
 * and launches either the @ref Server or the @ref Client depending on the first
 * positional argument (@c server / @c client).  A third mode, @c genconfig,
 * writes a default configuration file and exits.
 *
 * ### Invocation Syntax
 * @code
 * safesocket server [options]     Start as a multi-client server
 * safesocket client [options]     Connect to a server as a client
 * safesocket genconfig [path]     Write a default .conf file and exit
 * @endcode
 *
 * ### Argument Precedence
 * Values are applied in the following order (later stages override earlier ones):
 *  1. Compiled-in defaults (struct member initialisers in @ref Config).
 *  2. Configuration file (loaded via @ref Config::load()).
 *  3. Command-line flags (@c --host, @c --port, etc.).
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#include "config.hpp"
#include "network.hpp"   ///< Must be included before <windows.h> to ensure correct Winsock2 inclusion order.
#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
  #include <windows.h>

  /// @cond INTERNAL
  // Dev-C++ / old MinGW may not define ENABLE_VIRTUAL_TERMINAL_PROCESSING.
  #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
  #endif
  /// @endcond
#endif

// ============================================================
//  Windows ANSI Colour Initialisation
// ============================================================

#ifdef _WIN32
/**
 * @brief Enable ANSI/VT100 escape-code processing in the Windows console.
 *
 * Attempts to set the @c ENABLE_VIRTUAL_TERMINAL_PROCESSING console mode flag,
 * which is required for ANSI colour output to work on Windows 10 and later.
 *
 * If the call fails (e.g. on older Windows versions or when output is
 * redirected to a non-console handle), @c g_config.color_output is set to
 * @c false so that subsequent output omits escape sequences entirely.
 *
 * @note This function is compiled only on Windows targets.
 */
static void enable_ansi_colors_windows() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        g_config.color_output = false;
        return;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        g_config.color_output = false;
        return;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        g_config.color_output = false;
        return;
    }
}
#endif

// ============================================================
//  Startup Banner
// ============================================================

/**
 * @brief Print the SafeSocket ASCII-art banner to @c stdout.
 *
 * When @c g_config.color_output is @c true the banner is wrapped in bold cyan
 * ANSI escape codes.  When colour is disabled a plain version is printed that
 * is identical in content but free of escape sequences, so it displays
 * correctly in terminals that do not support ANSI codes or when output is
 * redirected.
 */
static void print_banner() {
    if (!g_config.color_output) {
        std::cout <<
        "  ____         __      ____             __        __ \n"
        " / __/__ _____/ /___  / __/__  _______/ /_____  / /_\n"
        "/ _// _ `/ __/ / _ \\ _\\ \\/ _ \\/ __/  '_/ -_) / __/\n"
        "/___/\\_,_/_/ /_/\\___//___/\\___/\\__/_/\\_\\\\__/ \\__/ \n"
        "  Cross-platform C++11 | Windows + Linux\n\n";
        return;
    }

    std::cout <<
    "\033[1m\033[36m"
    "  ____         __      ____             __        __ \n"
    " / __/__ _____/ /___  / __/__  _______/ /_____  / /_\n"
    "/ _// _ `/ __/ / _ \\ _\\ \\/ _ \\/ __/  '_/ -_) / __/\n"
    "/___/\\_,_/_/ /_/\\___//___/\\___/\\__/_/\\_\\\\__/ \\__/ \n"
    "\033[0m"
    "  Cross-platform C++11 | Windows + Linux\n\n";
}

// ============================================================
//  Usage / Help
// ============================================================

/**
 * @brief Print the command-line usage instructions to @c stdout.
 *
 * Describes every supported mode and flag with a short explanation and
 * example invocations.  Called when the user passes @c --help or when an
 * unrecognised mode is supplied.
 *
 * @param prog  The @c argv[0] string (name of the executable) inserted into
 *              the usage examples.
 */
static void print_usage(const char* prog) {
    std::cout <<
        "Usage:\n"
        "  " << prog << " server [options]      - Start as server\n"
        "  " << prog << " client [options]      - Start as client\n"
        "  " << prog << " genconfig             - Generate default config file\n\n"
        "Options (override config file):\n"
        "  --host <ip>            Server IP (default: 127.0.0.1)\n"
        "  --port <port>          Port number (default: 9000)\n"
        "  --nick <name>          Nickname (client mode)\n"
        "  --encrypt <type>       Encryption: NONE|XOR|VIGENERE|RC4\n"
        "  --key <passphrase>     Encryption/access key\n"
        "  --config <file>        Config file to load (default: safesocket.conf)\n"
        "  --download-dir <dir>   Directory for received files\n"
        "  --log <file>           Log output to file\n"
        "  --verbose              Enable verbose output\n"
        "  --no-color             Disable colored output\n"
        "  --max-clients <n>      Max clients (server mode)\n"
        "  --require-key          Require access key to join (server mode)\n"
        "  --motd <text>          Message of the Day (server mode)\n"
        "  --server-name <name>   Server display name\n"
        "  --help                 Show this help\n\n"
        "Examples:\n"
        "  " << prog << " server --port 9000 --encrypt RC4 --key mypassword\n"
        "  " << prog << " client --host 127.0.0.1 --port 9000 --nick Alice --encrypt RC4 --key mypassword\n"
        "  " << prog << " genconfig\n\n";
}

// ============================================================
//  main
// ============================================================

/**
 * @brief Application entry point.
 *
 * Execution flow:
 *  1. On Windows, attempt to enable ANSI colour via @ref enable_ansi_colors_windows().
 *  2. Print the startup banner via @ref print_banner().
 *  3. Validate that a mode argument was supplied; print usage and exit if not.
 *  4. Handle the special @c genconfig mode immediately (write defaults, print
 *     config, exit).
 *  5. Scan @c argv for @c --config to find the config file path before the full
 *     argument parse, then call @ref Config::load().
 *  6. Iterate @c argv a second time to apply all @c --flag overrides to
 *     @ref g_config.
 *  7. Call @ref net_init(); exit on failure.
 *  8. In @c server mode: warn about misconfigured security options, construct a
 *     @ref Server, call @ref Server::start(), then @ref Server::run_cli().
 *  9. In @c client mode: warn about misconfigured encryption, construct a
 *     @ref Client, call @ref Client::connect(), then @ref Client::run_cli().
 * 10. Call @ref net_cleanup() and return the exit code.
 *
 * @param argc  Number of command-line arguments (including @c argv[0]).
 * @param argv  Array of null-terminated argument strings.
 * @return      @c 0 on clean exit, @c 1 if startup or connection failed.
 */
int main(int argc, char** argv) {
#ifdef _WIN32
    /* Attempt ANSI colour support; disables colour automatically on failure. */
    enable_ansi_colors_windows();
#endif

    print_banner();

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    /* Normalise the mode string to lowercase for case-insensitive comparison. */
    std::string mode = argv[1];
    std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

    if (mode == "--help" || mode == "-h" || mode == "help") {
        print_usage(argv[0]);
        return 0;
    }

    // --------------------------------------------------------
    //  genconfig mode
    // --------------------------------------------------------

    /**
     * @brief Handle the @c genconfig subcommand.
     *
     * Saves current (default) configuration to the specified path (or
     * @c "safesocket.conf" if none is given), then prints the configuration
     * and exits.
     */
    if (mode == "genconfig") {
        std::string path = "safesocket.conf";
        if (argc >= 3) path = argv[2];
        if (g_config.save(path)) {
            std::cout << "[config] Default configuration saved to: " << path << "\n";
            g_config.print();
        } else {
            std::cerr << "[config] Failed to write: " << path << "\n";
            return 1;
        }
        return 0;
    }

    // --------------------------------------------------------
    //  Config file loading (first pass: find --config)
    // --------------------------------------------------------

    /**
     * @brief First-pass scan for the @c --config flag.
     *
     * Scanned before the full argument loop so that the correct file is loaded
     * before any @c --flag overrides are applied.  Defaults to
     * @c "safesocket.conf" when not specified.
     */
    std::string config_file = "safesocket.conf";
    for (int i = 2; i < argc - 1; ++i) {
        if (strcmp(argv[i], "--config") == 0) {
            config_file = argv[i + 1];
            break;
        }
    }

    /* Non-fatal if the config file is absent; compiled-in defaults are used. */
    if (g_config.load(config_file)) {
        std::cout << "[config] Loaded: " << config_file << "\n";
    }

    // --------------------------------------------------------
    //  Command-line override parsing (second pass)
    // --------------------------------------------------------

    /**
     * @brief Apply command-line flags to @ref g_config.
     *
     * Flags override values loaded from the configuration file.  Each flag
     * that takes a value consumes the following @c argv element.  Unknown
     * flags beginning with @c '-' are reported to @c stderr.
     */
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--host"         && i + 1 < argc) { g_config.host = argv[++i]; }
        else if (arg == "--port"    && i + 1 < argc) { g_config.port = std::stoi(argv[++i]); }
        else if (arg == "--nick"    && i + 1 < argc) { g_config.nickname = argv[++i]; }
        else if (arg == "--encrypt" && i + 1 < argc) { g_config.encrypt_type = encrypt_type_from_string(argv[++i]); }
        else if (arg == "--key"     && i + 1 < argc) { g_config.encrypt_key = argv[++i]; }
        else if (arg == "--config"  && i + 1 < argc) { ++i; /* Already handled above; skip the value. */ }
        else if (arg == "--download-dir" && i + 1 < argc) { g_config.download_dir = argv[++i]; }
        else if (arg == "--log"     && i + 1 < argc) { g_config.log_to_file = true; g_config.log_file = argv[++i]; }
        else if (arg == "--verbose")      { g_config.verbose = true; }
        else if (arg == "--no-color")     { g_config.color_output = false; }
        else if (arg == "--max-clients" && i + 1 < argc) { g_config.max_clients = std::stoi(argv[++i]); }
        else if (arg == "--require-key")  { g_config.require_key = true; }
        else if (arg == "--access-key" && i + 1 < argc) { g_config.access_key = argv[++i]; }
        else if (arg == "--motd"    && i + 1 < argc) { g_config.motd = argv[++i]; }
        else if (arg == "--server-name" && i + 1 < argc) { g_config.server_name = argv[++i]; }
        else if (arg == "--help")         { print_usage(argv[0]); return 0; }
        else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
        }
    }

    // --------------------------------------------------------
    //  Networking initialisation
    // --------------------------------------------------------

    /**
     * @brief Initialise the networking subsystem.
     *
     * On Windows this calls @c WSAStartup(); on POSIX it installs @c SIG_IGN
     * for @c SIGPIPE.  Any failure here is fatal.
     */
    if (!net_init()) {
        std::cerr << "Failed to initialize networking.\n";
        return 1;
    }

    int exit_code = 0;

    // --------------------------------------------------------
    //  Server mode
    // --------------------------------------------------------

    if (mode == "server") {
        /* Warn about common configuration mistakes before starting. */
        if (g_config.encrypt_type != EncryptType::NONE && g_config.encrypt_key.empty()) {
            std::cerr << "[warning] Encryption type set but no key provided.\n";
        }
        if (g_config.require_key && g_config.access_key.empty()) {
            std::cerr << "[warning] require_key=true but access_key is empty.\n";
        }

        Server server;
        if (!server.start(g_config.host, g_config.port)) {
            std::cerr << "Server failed to start.\n";
            exit_code = 1;
        } else {
            /* Blocks until the operator types /quit or stdin reaches EOF. */
            server.run_cli();
        }
    }

    // --------------------------------------------------------
    //  Client mode
    // --------------------------------------------------------

    else if (mode == "client") {
        if (g_config.encrypt_type != EncryptType::NONE && g_config.encrypt_key.empty()) {
            std::cerr << "[warning] Encryption type set but no key provided.\n";
        }

        Client client;
        if (!client.connect(g_config.host, g_config.port)) {
            std::cerr << "Client failed to connect.\n";
            exit_code = 1;
        } else {
            /* Blocks until the user types /quit or the connection drops. */
            client.run_cli();
        }
    }

    // --------------------------------------------------------
    //  Unknown mode
    // --------------------------------------------------------

    else {
        std::cerr << "Unknown mode: " << mode << "\n";
        print_usage(argv[0]);
        exit_code = 1;
    }

    net_cleanup();
    return exit_code;
}
