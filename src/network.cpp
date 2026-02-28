/**
 * @file network.cpp
 * @brief Implementation of the cross-platform socket abstraction layer.
 *
 * Provides the concrete implementations for all functions declared in
 * @ref network.hpp.  Platform-specific code paths are wrapped in
 * @c #ifdef _WIN32 / @c #else blocks so that a single source file compiles
 * cleanly on both Windows (Winsock2) and POSIX (BSD sockets) targets.
 *
 * Key implementation notes:
 *  - @ref net_send_raw() and @ref net_recv_raw() loop internally to handle
 *    partial sends/receives, guaranteeing that exactly the requested number of
 *    bytes is transferred or an error is returned.
 *  - @ref net_connect() uses a non-blocking @c connect() + @c select() pattern
 *    to honour the caller-supplied timeout without blocking indefinitely.
 *  - @ref net_send_packet() encrypts the payload before transmission.
 *  - @ref net_recv_packet() decrypts the payload after reception and validates
 *    the magic number in the header.
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#include "network.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
  /// @cond INTERNAL
  #pragma comment(lib, "Ws2_32.lib")  ///< Link against the Winsock2 import library automatically.
  /// @endcond
#else
  #include <sys/time.h>
  #include <signal.h>
#endif

// ============================================================
//  Initialisation / Cleanup
// ============================================================

/**
 * @brief Initialise the underlying networking library.
 *
 * On Windows: calls @c WSAStartup(MAKEWORD(2,2), &wsa) to load Winsock 2.2.
 * Returns @c false and prints an error if @c WSAStartup fails.
 *
 * On POSIX: installs @c SIG_IGN for @c SIGPIPE so that writes to a closed
 * peer socket return @c EPIPE rather than delivering an unhandled signal that
 * would terminate the process.
 *
 * @return @c true on success; @c false only on Windows when @c WSAStartup fails.
 */
bool net_init() {
#ifdef _WIN32
    WSADATA wsa;
    int r = WSAStartup(MAKEWORD(2,2), &wsa);
    if (r != 0) {
        std::cerr << "[net] WSAStartup failed: " << r << "\n";
        return false;
    }
#else
    // Ignore SIGPIPE so writes to closed sockets don't crash us
    signal(SIGPIPE, SIG_IGN);
#endif
    return true;
}

/**
 * @brief Release resources held by the networking library.
 *
 * On Windows: calls @c WSACleanup().
 * On POSIX: this function is a no-op.
 */
void net_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// ============================================================
//  Error Reporting
// ============================================================

/**
 * @brief Return a human-readable description of the most recent socket error.
 *
 * On Windows: retrieves the error code with @c WSAGetLastError() and converts
 * it to a string using @c FormatMessageA().
 *
 * On POSIX: uses @c strerror(errno).
 *
 * In both cases the numeric error code is appended in parentheses.
 *
 * @return A string of the form @c "Human-readable description (N)".
 */
std::string net_error_str() {
#ifdef _WIN32
    int code = WSAGetLastError();
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, code, 0, buf, sizeof(buf), NULL);
    std::string s(buf);
    /* Strip trailing whitespace that FormatMessageA often appends. */
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s + " (" + std::to_string(code) + ")";
#else
    return std::string(strerror(errno)) + " (" + std::to_string(errno) + ")";
#endif
}

// ============================================================
//  Socket Option Setters
// ============================================================

/**
 * @brief Enable @c SO_REUSEADDR on the given socket.
 *
 * Allows the server to rebind to the same port immediately after a restart
 * without waiting for the operating system to release the port from the
 * @c TIME_WAIT state.
 *
 * @param s  Socket to configure.
 * @return   @c true if @c setsockopt() succeeded, @c false otherwise.
 */
bool net_set_reuseaddr(sock_t s) {
    int opt = 1;
    return setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                      (const char*)&opt, sizeof(opt)) == 0;
}

/**
 * @brief Enable @c TCP_NODELAY (disable Nagle's algorithm) on the given socket.
 *
 * Reduces latency for small, frequent sends by transmitting packets immediately
 * without coalescing them in the Nagle buffer.
 *
 * @param s  Socket to configure.
 * @return   @c true if @c setsockopt() succeeded, @c false otherwise.
 */
bool net_set_nodelay(sock_t s) {
    int opt = 1;
    return setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                      (const char*)&opt, sizeof(opt)) == 0;
}

/**
 * @brief Toggle a socket between blocking and non-blocking I/O mode.
 *
 * On Windows: uses @c ioctlsocket() with @c FIONBIO.
 * On POSIX: uses @c fcntl() with @c F_GETFL / @c F_SETFL and the @c O_NONBLOCK
 * flag.
 *
 * @param s        Socket to configure.
 * @param nonblock @c true to enable non-blocking mode, @c false for blocking mode.
 * @return         @c true on success, @c false if the system call failed.
 */
bool net_set_nonblocking(sock_t s, bool nonblock) {
#ifdef _WIN32
    u_long mode = nonblock ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return false;
    if (nonblock) flags |= O_NONBLOCK;
    else          flags &= ~O_NONBLOCK;
    return fcntl(s, F_SETFL, flags) == 0;
#endif
}

// ============================================================
//  Server-Side: Listen and Accept
// ============================================================

/**
 * @brief Create a TCP listening socket bound to the specified address and port.
 *
 * Steps:
 *  1. Create an @c AF_INET / @c SOCK_STREAM / @c IPPROTO_TCP socket.
 *  2. Apply @c SO_REUSEADDR and @c TCP_NODELAY.
 *  3. Populate a @c sockaddr_in and call @c bind().
 *  4. Call @c listen() with the given @p backlog.
 *
 * An empty @p host or @c "0.0.0.0" binds to all interfaces (@c INADDR_ANY).
 *
 * @param host     IPv4 address string to bind to.
 * @param port     TCP port number.
 * @param backlog  Maximum pending-connection queue length.
 * @return         A valid listening socket, or @ref INVALID_SOCK on any failure.
 */
sock_t net_listen(const std::string& host, int port, int backlog) {
    sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCK) {
        std::cerr << "[net] socket() failed: " << net_error_str() << "\n";
        return INVALID_SOCK;
    }
    net_set_reuseaddr(s);
    net_set_nodelay(s);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);

    if (host.empty() || host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "[net] Invalid host: " << host << "\n";
            sock_close(s);
            return INVALID_SOCK;
        }
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCK_ERR) {
        std::cerr << "[net] bind() failed: " << net_error_str() << "\n";
        sock_close(s);
        return INVALID_SOCK;
    }

    if (listen(s, backlog) == SOCK_ERR) {
        std::cerr << "[net] listen() failed: " << net_error_str() << "\n";
        sock_close(s);
        return INVALID_SOCK;
    }
    return s;
}

/**
 * @brief Accept an incoming client connection from a listening socket.
 *
 * Blocks until a client connects.  On success, @c TCP_NODELAY is applied to
 * the accepted socket and the remote IPv4 address is written to @p peer_ip via
 * @c inet_ntop().
 *
 * @param[in]  server_sock  Listening socket returned by @ref net_listen().
 * @param[out] peer_ip      Set to the dotted-decimal IP address of the remote endpoint.
 * @return                  A valid connected socket, or @ref INVALID_SOCK on failure.
 */
sock_t net_accept(sock_t server_sock, std::string& peer_ip) {
    struct sockaddr_in addr;
#ifdef _WIN32
    int addrlen = sizeof(addr);
#else
    socklen_t addrlen = sizeof(addr);
#endif
    sock_t cs = accept(server_sock, (struct sockaddr*)&addr, &addrlen);
    if (cs == INVALID_SOCK) return INVALID_SOCK;

    /* Convert the binary address to a human-readable dotted-decimal string. */
    char ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    peer_ip = ip;
    net_set_nodelay(cs);
    return cs;
}

// ============================================================
//  Client-Side: Connect
// ============================================================

/**
 * @brief Create a TCP socket and connect it to a remote server with a timeout.
 *
 * Algorithm:
 *  1. Create an @c AF_INET socket and set @c TCP_NODELAY.
 *  2. Switch the socket to non-blocking mode.
 *  3. Call @c connect(); on POSIX/Windows this returns immediately with
 *     @c EINPROGRESS / @c WSAEWOULDBLOCK for a connection in progress.
 *  4. Use @c select() to wait up to @p timeout_sec seconds for writability,
 *     which signals that the connection completed or failed.
 *  5. Use @c getsockopt(SO_ERROR) to confirm the connection succeeded.
 *  6. Restore blocking mode before returning.
 *
 * @param host         Dotted-decimal IPv4 address of the server.
 * @param port         TCP port number of the server.
 * @param timeout_sec  Maximum seconds to wait for the connection to be established.
 * @return             A valid connected socket, or @ref INVALID_SOCK on failure or timeout.
 */
sock_t net_connect(const std::string& host, int port, int timeout_sec) {
    sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCK) return INVALID_SOCK;
    net_set_nodelay(s);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "[net] Invalid host: " << host << "\n";
        sock_close(s);
        return INVALID_SOCK;
    }

    /* Switch to non-blocking so connect() returns immediately. */
    net_set_nonblocking(s, true);

    int rc = connect(s, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == SOCK_ERR) {
#ifdef _WIN32
        int err = WSAGetLastError();
        bool in_progress = (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS);
#else
        bool in_progress = (errno == EINPROGRESS || errno == EWOULDBLOCK);
#endif
        if (!in_progress) {
            std::cerr << "[net] connect() failed: " << net_error_str() << "\n";
            sock_close(s);
            return INVALID_SOCK;
        }

        /* Wait for the socket to become writable, indicating connection completion. */
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(s, &wfds);
        struct timeval tv;
        tv.tv_sec  = timeout_sec;
        tv.tv_usec = 0;

        rc = select((int)s + 1, NULL, &wfds, NULL, &tv);
        if (rc <= 0) {
            std::cerr << "[net] connect() timed out\n";
            sock_close(s);
            return INVALID_SOCK;
        }

        /* Verify that the pending connect actually succeeded. */
        int err2 = 0;
#ifdef _WIN32
        int elen = sizeof(err2);
#else
        socklen_t elen = sizeof(err2);
#endif
        getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err2, &elen);
        if (err2 != 0) {
            std::cerr << "[net] connect() error: " << err2 << "\n";
            sock_close(s);
            return INVALID_SOCK;
        }
    }

    /* Restore blocking mode for all subsequent I/O. */
    net_set_nonblocking(s, false);
    return s;
}

// ============================================================
//  Low-Level Byte-Stream I/O
// ============================================================

/**
 * @brief Reliably transmit exactly @p len bytes over a connected socket.
 *
 * Loops calling @c send() until all @p len bytes have been delivered.
 * On POSIX, @c MSG_NOSIGNAL is passed to prevent delivery of @c SIGPIPE
 * on a broken connection.
 *
 * @param s    Connected socket to write to.
 * @param data Pointer to the source buffer.
 * @param len  Number of bytes to transmit.
 * @return     @c true if all bytes were sent; @c false if @c send() returned
 *             an error or the connection was closed by the remote end.
 */
bool net_send_raw(sock_t s, const void* data, size_t len) {
    const char* ptr = reinterpret_cast<const char*>(data);
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = send(s, ptr + sent, (int)(len - sent), 0);
#else
        int n = send(s, ptr + sent, len - sent, MSG_NOSIGNAL);
#endif
        if (n <= 0) return false;
        sent += (size_t)n;
    }
    return true;
}

/**
 * @brief Reliably receive exactly @p len bytes from a connected socket.
 *
 * Loops calling @c recv() until the full @p len bytes have been read into
 * @p buf.  Returns @c false immediately if @c recv() reports a connection
 * closure or error (return value = 0).
 *
 * @param s    Connected socket to read from.
 * @param buf  Destination buffer; must be at least @p len bytes.
 * @param len  Exact number of bytes to receive.
 * @return     @c true if exactly @p len bytes were placed in @p buf;
 *             @c false on any error or disconnection.
 */
bool net_recv_raw(sock_t s, void* buf, size_t len) {
    char* ptr = reinterpret_cast<char*>(buf);
    size_t got = 0;
    while (got < len) {
        int n = recv(s, ptr + got, (int)(len - got), 0);
        if (n <= 0) return false;
        got += (size_t)n;
    }
    return true;
}

// ============================================================
//  Protocol-Framed Packet I/O
// ============================================================

/**
 * @brief Serialize, optionally encrypt, and transmit a complete SafeSocket packet.
 *
 * If @p data is non-null and @p data_len is greater than zero, the payload is
 * copied into a local @c std::vector and encrypted in-place with @ref crypto_encrypt().
 * The @ref PacketHeader is then populated (including the post-encryption
 * @c data_len) and transmitted via @ref net_send_raw(), followed by the payload.
 *
 * @param s        Destination connected socket.
 * @param type     @ref MsgType value for the @c msg_type field.
 * @param sender   Originator ID for the @c sender_id field.
 * @param target   Recipient ID for the @c target_id field.
 * @param data     Raw payload bytes to attach; may be @c nullptr if @p data_len is 0.
 * @param data_len Number of bytes in @p data.
 * @param enc      Encryption algorithm to apply (default @c EncryptType::NONE).
 * @param key      Encryption passphrase (default empty string).
 * @return         @c true if the header and full payload were sent, @c false on any error.
 */
bool net_send_packet(sock_t s,
                     uint32_t type,
                     uint32_t sender,
                     uint32_t target,
                     const void* data,
                     uint32_t data_len,
                     EncryptType enc,
                     const std::string& key)
{
    /* Build a mutable copy of the payload so we can encrypt it in place. */
    std::vector<uint8_t> payload;
    if (data && data_len > 0) {
        payload.assign((const uint8_t*)data,
                       (const uint8_t*)data + data_len);
        crypto_encrypt(payload, enc, key);
    }

    /* Fill in the header using the final (post-encryption) payload size. */
    PacketHeader hdr;
    hdr.magic     = MAGIC;
    hdr.msg_type  = type;
    hdr.sender_id = sender;
    hdr.target_id = target;
    hdr.data_len  = (uint32_t)payload.size();

    if (!net_send_raw(s, &hdr, HEADER_SIZE)) return false;
    if (!payload.empty())
        if (!net_send_raw(s, payload.data(), payload.size())) return false;
    return true;
}

/**
 * @brief Receive and deserialize a complete SafeSocket packet.
 *
 * Steps:
 *  1. Receive exactly @ref HEADER_SIZE bytes into @p hdr via @ref net_recv_raw().
 *  2. Validate @c hdr.magic against @ref MAGIC; on mismatch print an error and
 *     return @c false — this typically indicates a desynchronised stream.
 *  3. Resize @p payload to @c hdr.data_len and receive that many bytes.
 *  4. Decrypt the payload in-place with @ref crypto_decrypt().
 *
 * @param[in]  s        Connected socket to read from.
 * @param[out] hdr      Populated with the received packet header.
 * @param[out] payload  Populated with the decrypted payload bytes.
 * @param[in]  enc      Decryption algorithm (must match the sender's encryption).
 * @param[in]  key      Decryption passphrase (must match the sender's key).
 * @return              @c true on success, @c false on I/O error, connection closure,
 *                      or magic-number mismatch.
 */
bool net_recv_packet(sock_t s,
                     PacketHeader& hdr,
                     std::vector<uint8_t>& payload,
                     EncryptType enc,
                     const std::string& key)
{
    if (!net_recv_raw(s, &hdr, HEADER_SIZE)) return false;
    if (hdr.magic != MAGIC) {
        std::cerr << "[net] Bad magic in packet header\n";
        return false;
    }
    payload.resize(hdr.data_len);
    if (hdr.data_len > 0) {
        if (!net_recv_raw(s, payload.data(), hdr.data_len)) return false;
        crypto_decrypt(payload, enc, key);
    }
    return true;
}

// ============================================================
//  Display Utilities
// ============================================================

/**
 * @brief Format a raw byte count as a human-readable string.
 *
 * Iteratively divides by 1024 until the value falls below 1024 or the
 * largest unit (TB) is reached.  Byte counts are printed as integers;
 * all larger units are formatted with two decimal places.
 *
 * Examples:
 *  - @c 512  ? @c "512 B"
 *  - @c 1536 ? @c "1.50 KB"
 *  - @c 1073741824 ? @c "1.00 GB"
 *
 * @param bytes  Raw byte count to format.
 * @return       A null-terminated string such as @c "1.50 MB".
 */
std::string fmt_bytes(uint64_t bytes) {
    const char* units[] = {"B","KB","MB","GB","TB"};
    int u = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && u < 4) { val /= 1024.0; ++u; }
    char buf[64];
    if (u == 0) snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    else        snprintf(buf, sizeof(buf), "%.2f %s", val, units[u]);
    return buf;
}
