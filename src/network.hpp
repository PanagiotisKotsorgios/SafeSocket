/**
 * @file network.hpp
 * @brief Cross-platform socket abstraction layer for SafeSocket.
 *
 * Provides a unified API for TCP networking on both Windows (Winsock2) and
 * POSIX (BSD sockets) platforms.  All platform-specific preprocessor guards,
 * type aliases, and macro definitions are contained in this header so that the
 * rest of the codebase can remain platform-agnostic.
 *
 * Key responsibilities:
 *  - Platform detection and conditional inclusion of socket headers.
 *  - Type aliases (@c sock_t, @c INVALID_SOCK, @c SOCK_ERR, @c sock_close).
 *  - Networking lifecycle functions: @ref net_init() and @ref net_cleanup().
 *  - Server-side helpers: @ref net_listen() and @ref net_accept().
 *  - Client-side helper: @ref net_connect().
 *  - Socket option setters: @ref net_set_nonblocking(), @ref net_set_nodelay(),
 *    @ref net_set_reuseaddr().
 *  - Reliable byte-stream I/O: @ref net_send_raw() and @ref net_recv_raw().
 *  - Protocol-aware framed I/O: @ref net_send_packet() and @ref net_recv_packet().
 *  - Diagnostics: @ref net_error_str() and @ref fmt_bytes().
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#pragma once

// ============================================================
//  Platform-specific socket includes and type aliases
// ============================================================

#ifdef _WIN32
  #ifndef _WIN32_WINNT
    /// @cond INTERNAL
    #define _WIN32_WINNT 0x0600   ///< Target Windows Vista or later for modern Winsock features.
    /// @endcond
  #endif
  #ifndef WINVER
    /// @cond INTERNAL
    #define WINVER 0x0600         ///< Minimum Windows version for API declarations.
    /// @endcond
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    /// @cond INTERNAL
    #define WIN32_LEAN_AND_MEAN   ///< Exclude rarely-used Windows headers to speed up compilation.
    /// @endcond
  #endif

  #include <winsock2.h>
  #include <ws2tcpip.h>

  /**
   * @typedef sock_t
   * @brief Platform-neutral socket handle type.
   *
   * On Windows this is @c SOCKET (an unsigned pointer-sized integer).
   * On POSIX it is @c int (a file descriptor).
   */
  typedef SOCKET sock_t;

  /** @brief Sentinel value representing an invalid or uninitialized socket handle. */
  #define INVALID_SOCK   INVALID_SOCKET

  /** @brief Return value indicating a socket API call failed. */
  #define SOCK_ERR       SOCKET_ERROR

  /**
   * @brief Close a socket handle in a platform-neutral way.
   * @param s The socket handle to close.
   */
  #define sock_close(s)  ::closesocket(s)

  /**
   * @brief Retrieve the most recent socket error code in a platform-neutral way.
   * @return The last socket error code.
   */
  #define sock_errno()   ::WSAGetLastError()

#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>

  /**
   * @typedef sock_t
   * @brief Platform-neutral socket handle type.
   *
   * On POSIX this is a plain @c int file descriptor.
   */
  typedef int sock_t;

  /** @brief Sentinel value representing an invalid or uninitialized socket file descriptor. */
  #define INVALID_SOCK  (-1)

  /** @brief Return value indicating a socket API call failed. */
  #define SOCK_ERR      (-1)

  /**
   * @brief Close a socket file descriptor in a platform-neutral way.
   * @param s The file descriptor to close.
   */
  #define sock_close(s) ::close(s)

  /**
   * @brief Retrieve the most recent socket error code in a platform-neutral way.
   * @return The value of @c errno after a failed socket call.
   */
  #define sock_errno()  errno
#endif

#include <string>
#include <vector>
#include <cstdint>
#include "protocol.hpp"
#include "crypto.hpp"
#include "config.hpp"

// ============================================================
//  Networking Lifecycle
// ============================================================

/**
 * @brief Initialise the networking subsystem.
 *
 * On Windows this calls @c WSAStartup() to load Winsock 2.2.
 * On POSIX it installs @c SIG_IGN for @c SIGPIPE so that writes to closed
 * sockets produce an error return rather than terminating the process.
 *
 * Must be called once before any other @c net_* function.
 *
 * @return @c true on success, @c false if initialisation failed (Windows only).
 */
bool net_init();

/**
 * @brief Tear down the networking subsystem.
 *
 * On Windows this calls @c WSACleanup().  On POSIX this is a no-op.
 * Should be called once before the process exits.
 */
void net_cleanup();

// ============================================================
//  Server-Side Functions
// ============================================================

/**
 * @brief Create a TCP listening socket bound to the given address and port.
 *
 * Sets @c SO_REUSEADDR and @c TCP_NODELAY on the created socket, then
 * calls @c bind() and @c listen().
 *
 * @param host     IPv4 address string to bind to, e.g. @c "0.0.0.0" or
 *                 @c "127.0.0.1".  An empty string is treated as @c INADDR_ANY.
 * @param port     TCP port number to bind to.
 * @param backlog  Maximum length of the pending-connection queue (default 128).
 * @return         A valid listening socket on success, or @ref INVALID_SOCK on failure.
 */
sock_t net_listen(const std::string& host, int port, int backlog = 128);

/**
 * @brief Accept an incoming connection on a listening socket.
 *
 * Blocks until a client connects or the call is interrupted.  Sets
 * @c TCP_NODELAY on the accepted socket before returning.
 *
 * @param[in]  server_sock  The listening socket returned by @ref net_listen().
 * @param[out] peer_ip      Populated with the dotted-decimal IPv4 address of the remote client.
 * @return                  A valid connected socket on success, or @ref INVALID_SOCK on failure.
 */
sock_t net_accept(sock_t server_sock, std::string& peer_ip);

// ============================================================
//  Client-Side Functions
// ============================================================

/**
 * @brief Create a TCP socket and connect it to a remote host.
 *
 * Uses a non-blocking @c connect() combined with @c select() to enforce
 * the specified connection timeout.  The socket is restored to blocking
 * mode before being returned.
 *
 * @param host         Dotted-decimal IPv4 address of the server to connect to.
 * @param port         TCP port number of the server.
 * @param timeout_sec  Maximum seconds to wait for the connection to complete (default 10).
 * @return             A valid connected socket on success, or @ref INVALID_SOCK on failure.
 */
sock_t net_connect(const std::string& host, int port, int timeout_sec = 10);

// ============================================================
//  Socket Option Setters
// ============================================================

/**
 * @brief Toggle a socket between blocking and non-blocking I/O mode.
 *
 * Uses @c ioctlsocket() on Windows and @c fcntl() with @c O_NONBLOCK on POSIX.
 *
 * @param s        The socket to configure.
 * @param nonblock @c true to enable non-blocking mode, @c false for blocking mode.
 * @return         @c true on success, @c false if the call failed.
 */
bool net_set_nonblocking(sock_t s, bool nonblock);

/**
 * @brief Enable @c TCP_NODELAY (disable Nagle's algorithm) on a socket.
 *
 * Reduces latency for small, frequent sends by bypassing the Nagle buffer.
 *
 * @param s  The socket to configure.
 * @return   @c true on success, @c false if @c setsockopt() failed.
 */
bool net_set_nodelay(sock_t s);

/**
 * @brief Enable @c SO_REUSEADDR on a socket.
 *
 * Allows the server to rebind to the same port immediately after a restart
 * without waiting for the OS to release it from the @c TIME_WAIT state.
 *
 * @param s  The socket to configure.
 * @return   @c true on success, @c false if @c setsockopt() failed.
 */
bool net_set_reuseaddr(sock_t s);

// ============================================================
//  Low-Level Byte-Stream I/O
// ============================================================

/**
 * @brief Reliably send exactly @p len bytes over a connected socket.
 *
 * Loops internally to handle partial sends.  Uses @c MSG_NOSIGNAL on POSIX
 * to prevent @c SIGPIPE.
 *
 * @param s    Connected socket to write to.
 * @param data Pointer to the data buffer to transmit.
 * @param len  Number of bytes to transmit from @p data.
 * @return     @c true if all bytes were sent, @c false on any send error or disconnection.
 */
bool net_send_raw(sock_t s, const void* data, size_t len);

/**
 * @brief Reliably receive exactly @p len bytes from a connected socket.
 *
 * Loops internally to handle partial receives.
 *
 * @param s    Connected socket to read from.
 * @param buf  Buffer that will receive the incoming bytes; must be at least @p len bytes.
 * @param len  Exact number of bytes to read.
 * @return     @c true if exactly @p len bytes were received, @c false on error or disconnection.
 */
bool net_recv_raw(sock_t s, void* buf, size_t len);

// ============================================================
//  Protocol-Framed Packet I/O
// ============================================================

/**
 * @brief Serialize and send a complete SafeSocket packet.
 *
 * Optionally encrypts @p data using @ref crypto_encrypt() before transmission.
 * Always sends the @ref PacketHeader first, followed by the (possibly encrypted)
 * payload.
 *
 * @param s        Destination connected socket.
 * @param type     @ref MsgType value for the @c msg_type header field.
 * @param sender   Originator ID for the @c sender_id header field.
 * @param target   Recipient ID for the @c target_id header field.
 * @param data     Pointer to the raw payload bytes; may be @c nullptr if @p data_len is 0.
 * @param data_len Number of bytes pointed to by @p data.
 * @param enc      Encryption algorithm to apply to the payload (default @c EncryptType::NONE).
 * @param key      Passphrase used by the chosen encryption algorithm (default empty string).
 * @return         @c true if the entire packet was sent successfully, @c false otherwise.
 */
bool net_send_packet(sock_t s,
                     uint32_t type,
                     uint32_t sender,
                     uint32_t target,
                     const void* data,
                     uint32_t data_len,
                     EncryptType enc = EncryptType::NONE,
                     const std::string& key = "");

/**
 * @brief Receive and deserialize a complete SafeSocket packet.
 *
 * Reads a @ref PacketHeader, validates the magic number, reads the payload
 * indicated by @c data_len, and then decrypts it using @ref crypto_decrypt().
 *
 * @param[in]  s        Connected socket to read from.
 * @param[out] hdr      Populated with the received @ref PacketHeader.
 * @param[out] payload  Populated with the decrypted payload bytes.
 * @param[in]  enc      Decryption algorithm to apply (must match the sender's encryption).
 * @param[in]  key      Passphrase used for decryption (must match the sender's key).
 * @return              @c true on success, @c false on I/O error, disconnection, or bad magic.
 */
bool net_recv_packet(sock_t s,
                     PacketHeader& hdr,
                     std::vector<uint8_t>& payload,
                     EncryptType enc = EncryptType::NONE,
                     const std::string& key = "");

// ============================================================
//  Diagnostics
// ============================================================

/**
 * @brief Return a human-readable description of the most recent socket error.
 *
 * On Windows uses @c FormatMessageA() with the last @c WSAGetLastError() code.
 * On POSIX uses @c strerror() with @c errno.
 *
 * @return String of the form @c "Error description (code)".
 */
std::string net_error_str();

/**
 * @brief Format a byte count as a human-readable string with the appropriate unit.
 *
 * Automatically selects B, KB, MB, GB, or TB and formats with two decimal places
 * for any unit larger than bytes.
 *
 * @param bytes  Raw byte count to format.
 * @return       Formatted string, e.g. @c "1.50 MB" or @c "42 B".
 */
std::string fmt_bytes(uint64_t bytes);
