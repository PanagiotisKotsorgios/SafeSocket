/**
 * @file server.cpp
 * @brief Implementation of the SafeSocket multi-client TCP server.
 *
 * Provides the full implementation for @ref Server as declared in @ref server.hpp.
 * The server supports:
 *  - Concurrent clients via per-client detached threads (@ref Server::client_loop).
 *  - An accept thread (@ref Server::accept_loop) that enqueues new connections.
 *  - An optional keepalive thread (@ref Server::keepalive_loop) that periodically
 *    pings all connected clients.
 *  - Broadcast and private text messaging.
 *  - Bidirectional file transfer (client?server upload and server?client push).
 *  - Access-key authentication, nickname management, and client kicking.
 *  - An interactive operator CLI (@ref Server::run_cli).
 *
 * ### File Transfer Flow (client ? server)
 * @code
 * Client  - MSG_FILE_START --------------? Server
 *         ? MSG_FILE_ACCEPT --------------
 *         - MSG_FILE_DATA (N chunks) ----?
 *         - MSG_FILE_END ----------------?
 *         ? MSG_ACK ----------------------
 * @endcode
 *
 * ### File Transfer Flow (server ? client)
 * @code
 * Server  - MSG_FILE_START --------------? Client
 *         ? MSG_FILE_ACCEPT / MSG_FILE_REJECT ?
 *         - MSG_FILE_DATA (N chunks) ----?
 *         - MSG_FILE_END ----------------?
 * @endcode
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#include "server.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  /// @cond INTERNAL
  #define mkdir_compat(p) _mkdir(p)   ///< Platform-neutral directory creation (Windows uses @c _mkdir).
  /// @endcond
#else
  #include <unistd.h>
  /// @cond INTERNAL
  #define mkdir_compat(p) mkdir(p, 0755) ///< Platform-neutral directory creation (POSIX uses @c mkdir with mode).
  /// @endcond
#endif

// ============================================================
//  ANSI Colour Constants
// ============================================================

/** @brief ANSI escape code that resets all text attributes. */
static const char* CLR_RESET  = "\033[0m";
/** @brief ANSI escape code for cyan text (used for informational messages). */
static const char* CLR_CYAN   = "\033[36m";
/** @brief ANSI escape code for green text (used for success/join messages). */
static const char* CLR_GREEN  = "\033[32m";
/** @brief ANSI escape code for yellow text (used for warnings and disconnects). */
static const char* CLR_YELLOW = "\033[33m";
/** @brief ANSI escape code for red text (used for errors and failures). */
static const char* CLR_RED    = "\033[31m";
/** @brief ANSI escape code that enables bold/bright text. */
static const char* CLR_BOLD   = "\033[1m";
/** @brief ANSI escape code for magenta text (used for private messages). */
static const char* CLR_MAGENTA= "\033[35m";

/**
 * @brief Return @p c as a std::string, or an empty string if colour is disabled.
 *
 * Centralises the @c g_config.color_output check so that callers need not
 * repeat the conditional at every print site.
 *
 * @param c  ANSI escape sequence constant (e.g. @ref CLR_GREEN).
 * @return   @p c wrapped in a @c std::string, or @c "" when colour is disabled.
 */
static std::string clr(const char* c) {
    return g_config.color_output ? std::string(c) : "";
}

// ============================================================
//  Constructor / Destructor
// ============================================================

/**
 * @brief Construct an idle @ref Server with default-initialised state.
 *
 * The listening socket is set to @ref INVALID_SOCK, the running flag to
 * @c false, and all counters to zero.  Call @ref Server::start() to make
 * the server operational.
 */
Server::Server()
    : m_listen_sock(INVALID_SOCK), m_running(false),
      m_next_id(1), m_bytes_sent(0), m_bytes_recv(0), m_msg_count(0)
{}

/**
 * @brief Destroy the server, stopping it first if it is still running.
 *
 * Delegates to @ref Server::stop() to ensure all threads are joined and all
 * sockets are closed before the object is freed.
 */
Server::~Server() {
    stop();
}

// ============================================================
//  Logging Helpers
// ============================================================

/**
 * @brief Append a timestamped entry to the log file.
 *
 * The timestamp format is @c "YYYY-MM-DD HH:MM:SS".  Does nothing if
 * @c g_config.log_to_file is @c false or if the log file stream is not open.
 *
 * @param msg  The plain-text message to write (a newline is appended automatically).
 */
void Server::log(const std::string& msg) {
    if (g_config.log_to_file && m_log_file.is_open()) {
        time_t t = time(nullptr);
        char ts[32];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
        m_log_file << "[" << ts << "] " << msg << "\n";
        m_log_file.flush();
    }
}

/**
 * @brief Print a message to @c stdout, optionally in colour, and log it.
 *
 * When @p color is non-null and @c g_config.color_output is @c true the
 * message is wrapped in the ANSI colour sequence followed by @ref CLR_RESET.
 * The message is also forwarded to @ref Server::log().
 *
 * @param msg    The text to display and log.
 * @param color  ANSI colour escape sequence, or @c nullptr for plain output.
 */
void Server::print(const std::string& msg, const char* color) {
    if (color && g_config.color_output)
        std::cout << color << msg << CLR_RESET << "\n";
    else
        std::cout << msg << "\n";
    log(msg);
}

// ============================================================
//  Lifecycle: start / stop
// ============================================================

/**
 * @brief Initialise and start the server.
 *
 * Performs the following steps in order:
 *  1. Create the download directory with @c mkdir_compat().
 *  2. Open the log file if @c g_config.log_to_file is @c true.
 *  3. Call @ref net_listen() to bind and begin listening.
 *  4. Set @c m_running to @c true.
 *  5. Launch @ref accept_loop() on @c m_accept_thread.
 *  6. Conditionally launch @ref keepalive_loop() on @c m_keepalive_thread.
 *  7. Print the startup banner.
 *
 * @param host  IPv4 address to bind to.
 * @param port  TCP port to listen on.
 * @return      @c true on success, @c false if @ref net_listen() failed.
 */
bool Server::start(const std::string& host, int port) {
    mkdir_compat(g_config.download_dir.c_str());

    if (g_config.log_to_file) {
        m_log_file.open(g_config.log_file, std::ios::app);
    }

    m_listen_sock = net_listen(host, port);
    if (m_listen_sock == INVALID_SOCK) return false;

    m_running = true;
    m_accept_thread = std::thread(&Server::accept_loop, this);

    if (g_config.keepalive)
        m_keepalive_thread = std::thread(&Server::keepalive_loop, this);

    print(clr(CLR_BOLD) + clr(CLR_GREEN) +
          "==============================================\n"
          "  SafeSocket SERVER  |  " + g_config.server_name + "\n"
          "  Listening on " + host + ":" + std::to_string(port) + "\n"
          "  Encryption: " + encrypt_type_name(g_config.encrypt_type) + "\n"
          "==============================================");

    return true;
}

/**
 * @brief Gracefully stop the server and release all resources.
 *
 * Sets @c m_running to @c false, closes the listening socket to unblock the
 * accept thread, marks every client as not alive, closes all client sockets,
 * and joins both the accept and keepalive threads.  Finally iterates over
 * all client entries to join their threads and free heap memory.
 *
 * Safe to call multiple times; the second and subsequent calls are no-ops.
 */
void Server::stop() {
    if (!m_running) return;
    m_running = false;

    /* Close the listening socket to unblock accept_loop(). */
    if (m_listen_sock != INVALID_SOCK) {
        sock_close(m_listen_sock);
        m_listen_sock = INVALID_SOCK;
    }

    /* Signal and close all connected clients. */
    {
        std::lock_guard<std::mutex> lk(m_clients_mutex);
        for (auto& kv : m_clients) {
            kv.second->alive = false;
            sock_close(kv.second->sock);
        }
    }

    if (m_accept_thread.joinable()) m_accept_thread.join();
    if (m_keepalive_thread.joinable()) m_keepalive_thread.join();

    /* Join and free all client records. */
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    for (auto& kv : m_clients) {
        if (kv.second->thread.joinable())
            kv.second->thread.join();
        delete kv.second;
    }
    m_clients.clear();
    print("[server] Stopped.", CLR_YELLOW);
}

// ============================================================
//  Accept Loop
// ============================================================

/**
 * @brief Thread function that accepts new TCP connections in a loop.
 *
 * Runs on @c m_accept_thread until @c m_running becomes @c false.  For each
 * accepted connection:
 *  - If the client count is at @c g_config.max_clients, the connection is
 *    rejected with an @ref MSG_ERROR packet and the socket is closed.
 *  - Otherwise, a @ref ClientInfo is allocated, inserted into @c m_clients,
 *    and a detached thread is started for @ref client_loop().
 */
void Server::accept_loop() {
    while (m_running) {
        std::string peer_ip;
        sock_t cs = net_accept(m_listen_sock, peer_ip);
        if (cs == INVALID_SOCK) {
            if (!m_running) break;   /* Listening socket was closed by stop(). */
            continue;
        }

        std::lock_guard<std::mutex> lk(m_clients_mutex);
        if ((int)m_clients.size() >= g_config.max_clients) {
            /* Reject the connection — server is at capacity. */
            net_send_packet(cs, MSG_ERROR, SERVER_ID, 0,
                            "Server full", 11,
                            g_config.encrypt_type, g_config.encrypt_key);
            sock_close(cs);
            continue;
        }

        /* Allocate and initialise client state. */
        ClientInfo* ci = new ClientInfo();
        ci->id           = m_next_id++;
        ci->sock         = cs;
        ci->ip           = peer_ip;
        ci->nickname     = "client_" + std::to_string(ci->id);
        ci->connected_at = time(nullptr);
        ci->alive        = true;
        ci->authenticated= !g_config.require_key; /* Auto-auth when no key required. */

        m_clients[ci->id] = ci;
        ci->thread = std::thread(&Server::client_loop, this, ci);
        ci->thread.detach();

        print("[server] New connection from " + peer_ip +
              " assigned ID=" + std::to_string(ci->id), CLR_CYAN);
        log("CONNECT id=" + std::to_string(ci->id) + " ip=" + peer_ip);
    }
}

// ============================================================
//  Per-Client Receive Loop
// ============================================================

/**
 * @brief Receive and dispatch packets for a single connected client.
 *
 * This function runs on a dedicated thread for each client.  It:
 *  1. Sends the MOTD and the assigned client ID as @ref MSG_SERVER_INFO packets.
 *  2. Enters a loop calling @ref net_recv_packet() and dispatching via
 *     @ref handle_packet() until @c ci->alive is @c false or the connection drops.
 *  3. On exit, announces the disconnect, updates statistics, and calls
 *     @ref remove_client().
 *
 * @param ci  Pointer to the @ref ClientInfo for the client being served.
 */
void Server::client_loop(ClientInfo* ci) {
    /* Send the Message of the Day to the newly connected client. */
    net_send_packet(ci->sock, MSG_SERVER_INFO, SERVER_ID, ci->id,
                    g_config.motd.c_str(), (uint32_t)g_config.motd.size(),
                    g_config.encrypt_type, g_config.encrypt_key);

    /* Inform the client of its server-assigned numeric ID. */
    std::string id_msg = "YOUR_ID=" + std::to_string(ci->id);
    net_send_packet(ci->sock, MSG_SERVER_INFO, SERVER_ID, ci->id,
                    id_msg.c_str(), (uint32_t)id_msg.size(),
                    g_config.encrypt_type, g_config.encrypt_key);

    PacketHeader hdr;
    std::vector<uint8_t> payload;

    while (ci->alive && m_running) {
        hdr = PacketHeader{};
        payload.clear();

        /* Block until a complete packet is received or the connection drops. */
        if (!net_recv_packet(ci->sock, hdr, payload,
                             g_config.encrypt_type, g_config.encrypt_key))
        {
            break;
        }
        m_bytes_recv += HEADER_SIZE + payload.size();
        handle_packet(ci, hdr, payload);
    }

    print("[server] Client " + std::to_string(ci->id) +
          " (" + ci->nickname + ") disconnected.", CLR_YELLOW);
    log("DISCONNECT id=" + std::to_string(ci->id) +
        " nick=" + ci->nickname);

    /* Notify remaining clients that this peer has left. */
    std::string bye = ci->nickname + " left the server.";
    broadcast_text(bye);
    remove_client(ci->id);
}

// ============================================================
//  Packet Dispatcher
// ============================================================

/**
 * @brief Route a received packet to the appropriate handler based on its type.
 *
 * Handles the following message types:
 *  - @ref MSG_CONNECT: Set/validate nickname and access key; send client list; announce join.
 *  - @ref MSG_TEXT / @ref MSG_BROADCAST: Forward to all other clients.
 *  - @ref MSG_PRIVATE: Forward to the specified target client.
 *  - @ref MSG_CLIENT_LIST: Re-send the current client list.
 *  - @ref MSG_NICK_SET: Update the nickname and notify all clients.
 *  - @ref MSG_PING: Reply with @ref MSG_PONG.
 *  - @ref MSG_FILE_START: Delegate to @ref handle_file_transfer().
 *  - @ref MSG_DISCONNECT: Mark the client as no longer alive.
 *
 * Unknown types produce a verbose warning when @c g_config.verbose is @c true.
 *
 * @param ci       The sending client's @ref ClientInfo.
 * @param hdr      The parsed @ref PacketHeader of the received packet.
 * @param payload  The decrypted payload bytes of the received packet.
 */
void Server::handle_packet(ClientInfo* ci,
                            const PacketHeader& hdr,
                            const std::vector<uint8_t>& payload)
{
    /* Convenience: interpret the payload as a UTF-8 string for text-based messages. */
    std::string text(payload.begin(), payload.end());

    switch (hdr.msg_type) {
    case MSG_CONNECT:
        /* Payload layout: [uint16_t nick_len][nick bytes][optional access key bytes] */
        {
            if (!payload.empty()) {
                uint16_t nlen = 0;
                if (payload.size() >= 2) {
                    memcpy(&nlen, payload.data(), 2);
                    if (nlen > 0 && (size_t)(2 + nlen) <= payload.size())
                        ci->nickname = std::string((char*)payload.data() + 2, nlen);

                    /* Validate the access key when one is required. */
                    if (g_config.require_key) {
                        size_t key_start = 2 + nlen;
                        std::string provided_key = "";
                        if (key_start < payload.size())
                            provided_key = std::string((char*)payload.data() + key_start,
                                                       payload.size() - key_start);
                        if (provided_key != g_config.access_key) {
                            net_send_packet(ci->sock, MSG_ERROR, SERVER_ID, ci->id,
                                            "ACCESS DENIED: invalid key", 26,
                                            g_config.encrypt_type, g_config.encrypt_key);
                            ci->alive = false;
                            return;
                        }
                        ci->authenticated = true;
                    }
                }
            }
            print("[server] Client " + std::to_string(ci->id) +
                  " identified as [" + ci->nickname + "] from " + ci->ip, CLR_GREEN);

            /* Distribute the current peer list to the newly joined client. */
            send_client_list(ci->id);

            /* Inform every existing client that this peer has joined. */
            broadcast_text(ci->nickname + " joined the server!", ci->id);
            ++m_msg_count;
        }
        break;

    case MSG_TEXT:
    case MSG_BROADCAST:
        if (!ci->authenticated) break;
        {
            std::string out = "[" + ci->nickname + "] " + text;
            print(out, CLR_CYAN);
            ++m_msg_count;

            /* Forward to every other client; the sender sees their own message
               reflected back as a server broadcast. */
            std::lock_guard<std::mutex> lk(m_clients_mutex);
            for (auto& kv : m_clients) {
                if (kv.first != ci->id) {
                    net_send_packet(kv.second->sock, MSG_BROADCAST,
                                    ci->id, kv.first,
                                    out.c_str(), (uint32_t)out.size(),
                                    g_config.encrypt_type, g_config.encrypt_key);
                    m_bytes_sent += HEADER_SIZE + out.size();
                }
            }
        }
        break;

    case MSG_PRIVATE:
        if (!ci->authenticated) break;
        {
            uint32_t target = hdr.target_id;
            std::lock_guard<std::mutex> lk(m_clients_mutex);
            auto it = m_clients.find(target);
            if (it != m_clients.end()) {
                /* Build and forward the annotated private message. */
                std::string out = "[PM from " + ci->nickname + "] " + text;
                net_send_packet(it->second->sock, MSG_PRIVATE,
                                ci->id, target,
                                out.c_str(), (uint32_t)out.size(),
                                g_config.encrypt_type, g_config.encrypt_key);
                m_bytes_sent += HEADER_SIZE + out.size();
                print("[PM] " + ci->nickname + " -> " +
                      it->second->nickname + ": " + text, CLR_MAGENTA);
                ++m_msg_count;
            } else {
                /* Target not found — inform the sender. */
                std::string err = "Client ID " + std::to_string(target) + " not found.";
                net_send_packet(ci->sock, MSG_ERROR, SERVER_ID, ci->id,
                                err.c_str(), (uint32_t)err.size(),
                                g_config.encrypt_type, g_config.encrypt_key);
            }
        }
        break;

    case MSG_CLIENT_LIST:
        /* Client is explicitly requesting a refreshed peer list. */
        send_client_list(ci->id);
        break;

    case MSG_NICK_SET:
        {
            std::string old = ci->nickname;
            ci->nickname = text;
            print("[server] " + old + " is now known as " + ci->nickname, CLR_YELLOW);
            broadcast_text(old + " is now known as " + ci->nickname, SERVER_ID);
            send_client_list(0); /* Push the updated list to all clients. */
        }
        break;

    case MSG_PING:
        /* Respond immediately with a pong to keep the connection alive. */
        net_send_packet(ci->sock, MSG_PONG, SERVER_ID, ci->id,
                        nullptr, 0,
                        g_config.encrypt_type, g_config.encrypt_key);
        break;

    case MSG_FILE_START:
        /* Delegate to the dedicated file-receive handler. */
        handle_file_transfer(ci);
        break;

    case MSG_DISCONNECT:
        ci->alive = false;
        break;

    default:
        if (g_config.verbose)
            print("[server] Unknown packet type " +
                  std::to_string(hdr.msg_type), CLR_YELLOW);
        break;
    }
}

// ============================================================
//  File Transfer: Client ? Server (upload)
// ============================================================

/**
 * @brief Receive a file uploaded by a client.
 *
 * Called from @ref handle_packet() when a @ref MSG_FILE_START packet arrives.
 * This function performs a fresh @ref net_recv_packet() to read the file
 * metadata (because the payload of the triggering packet has already been
 * consumed by the dispatcher).
 *
 * Steps:
 *  1. Receive @ref MSG_FILE_START containing a @ref FileStartPayload and filename.
 *  2. Sanitize the filename to block directory-traversal characters.
 *  3. Open the output file in @c g_config.download_dir.
 *  4. Send @ref MSG_FILE_ACCEPT to the client.
 *  5. Loop receiving @ref MSG_FILE_DATA chunks and writing them to disk.
 *  6. Stop when @ref MSG_FILE_END is received.
 *  7. Send @ref MSG_ACK to confirm receipt.
 *
 * @param ci  The uploading client's @ref ClientInfo.
 */
void Server::handle_file_transfer(ClientInfo* ci) {
    /*
     * NOTE: handle_file_transfer() is invoked from handle_packet() after the
     * initial MSG_FILE_START header has already been read.  A fresh recv is
     * performed here to obtain the full file-start payload.
     */
    PacketHeader hdr;
    std::vector<uint8_t> payload;

    if (!net_recv_packet(ci->sock, hdr, payload,
                         g_config.encrypt_type, g_config.encrypt_key)) return;

    if (hdr.msg_type != MSG_FILE_START || payload.size() < sizeof(FileStartPayload))
        return;

    /* Parse the file metadata from the payload prefix. */
    FileStartPayload* fsp = (FileStartPayload*)payload.data();
    uint64_t file_size    = fsp->file_size;
    uint32_t fname_len    = fsp->filename_len;

    if (payload.size() < sizeof(FileStartPayload) + fname_len) return;
    std::string filename((char*)payload.data() + sizeof(FileStartPayload), fname_len);

    /* Sanitize: replace path-separator characters to prevent directory traversal. */
    for (char& c : filename)
        if (c == '/' || c == '\\' || c == ':') c = '_';

    std::string out_path = g_config.download_dir + "/" + filename;
    std::ofstream outf(out_path, std::ios::binary);
    if (!outf.is_open()) {
        print("[server] Cannot write file: " + out_path, CLR_RED);
        return;
    }

    print("[server] Receiving file '" + filename + "' (" +
          fmt_bytes(file_size) + ") from " + ci->nickname + "...", CLR_CYAN);

    /* Signal the client that the server is ready to receive data. */
    net_send_packet(ci->sock, MSG_FILE_ACCEPT, SERVER_ID, ci->id,
                    nullptr, 0,
                    g_config.encrypt_type, g_config.encrypt_key);

    uint64_t received = 0;
    bool ok = true;

    while (ok) {
        PacketHeader chdr;
        std::vector<uint8_t> cpayload;
        if (!net_recv_packet(ci->sock, chdr, cpayload,
                             g_config.encrypt_type, g_config.encrypt_key)) {
            ok = false; break;
        }
        if (chdr.msg_type == MSG_FILE_END) break;   /* Transfer complete. */
        if (chdr.msg_type == MSG_FILE_DATA) {
            outf.write((char*)cpayload.data(), cpayload.size());
            received += cpayload.size();
            m_bytes_recv += HEADER_SIZE + cpayload.size();

            /* Optionally show a progress percentage in verbose mode. */
            if (file_size > 0 && g_config.verbose) {
                int pct = (int)(received * 100 / file_size);
                std::cout << "\r[server] Progress: " << pct << "% ("
                          << fmt_bytes(received) << "/" << fmt_bytes(file_size) << ")  "
                          << std::flush;
            }
        }
    }
    outf.close();

    if (ok) {
        std::cout << "\n";
        print("[server] File saved: " + out_path + " (" +
              fmt_bytes(received) + ")", CLR_GREEN);
        log("FILE_RECV from=" + std::to_string(ci->id) +
            " file=" + filename + " size=" + std::to_string(received));

        std::string ack_msg = "File '" + filename + "' received by server.";
        net_send_packet(ci->sock, MSG_ACK, SERVER_ID, ci->id,
                        ack_msg.c_str(), (uint32_t)ack_msg.size(),
                        g_config.encrypt_type, g_config.encrypt_key);
    } else {
        print("[server] File transfer interrupted.", CLR_RED);
    }
}

// ============================================================
//  File Transfer: Server ? Client (push)
// ============================================================

/**
 * @brief Transmit a file from the server to a single client.
 *
 * Internal implementation shared by the public @ref Server::send_file() and
 * @ref Server::send_file_all() wrappers.
 *
 * Steps:
 *  1. Open and measure the file.
 *  2. Send @ref MSG_FILE_START with @ref FileStartPayload + filename.
 *  3. Wait for @ref MSG_FILE_ACCEPT; abort on @ref MSG_FILE_REJECT or error.
 *  4. Stream @ref MSG_FILE_DATA chunks of up to @ref FILE_CHUNK bytes each.
 *  5. Send @ref MSG_FILE_END.
 *
 * @param ci        Target client's @ref ClientInfo.
 * @param filepath  Absolute or relative path to the file to send.
 * @return          @c true on successful transmission, @c false on any failure.
 */
bool Server::send_file_to_client(ClientInfo* ci, const std::string& filepath) {
    /* Open file and measure its total size. */
    std::ifstream inf(filepath, std::ios::binary | std::ios::ate);
    if (!inf.is_open()) {
        print("[server] Cannot open file: " + filepath, CLR_RED);
        return false;
    }
    uint64_t file_size = (uint64_t)inf.tellg();
    inf.seekg(0);

    /* Extract the base filename from the full path for display and transfer. */
    std::string filename = filepath;
    size_t slash = filename.find_last_of("/\\");
    if (slash != std::string::npos) filename = filename.substr(slash + 1);

    print("[server] Sending file '" + filename + "' (" +
          fmt_bytes(file_size) + ") to " + ci->nickname + "...", CLR_CYAN);

    /* Build the MSG_FILE_START payload: FileStartPayload struct + filename bytes. */
    std::vector<uint8_t> start_payload(sizeof(FileStartPayload) + filename.size());
    FileStartPayload* fsp = (FileStartPayload*)start_payload.data();
    fsp->file_size    = file_size;
    fsp->filename_len = (uint32_t)filename.size();
    memcpy(start_payload.data() + sizeof(FileStartPayload),
           filename.c_str(), filename.size());

    if (!net_send_packet(ci->sock, MSG_FILE_START, SERVER_ID, ci->id,
                         start_payload.data(), (uint32_t)start_payload.size(),
                         g_config.encrypt_type, g_config.encrypt_key)) {
        print("[server] Failed to send FILE_START", CLR_RED);
        return false;
    }

    /* Wait for the client to accept or reject the incoming transfer. */
    PacketHeader ahdr;
    std::vector<uint8_t> apayload;
    if (!net_recv_packet(ci->sock, ahdr, apayload,
                         g_config.encrypt_type, g_config.encrypt_key)) {
        print("[server] Client did not accept file transfer.", CLR_RED);
        return false;
    }
    if (ahdr.msg_type == MSG_FILE_REJECT) {
        print("[server] Client rejected file transfer.", CLR_YELLOW);
        return false;
    }

    /* Stream file data in FILE_CHUNK-sized packets. */
    std::vector<uint8_t> chunk(FILE_CHUNK);
    uint64_t sent = 0;
    while (inf.read((char*)chunk.data(), FILE_CHUNK) || inf.gcount() > 0) {
        size_t n = (size_t)inf.gcount();
        if (!net_send_packet(ci->sock, MSG_FILE_DATA, SERVER_ID, ci->id,
                             chunk.data(), (uint32_t)n,
                             g_config.encrypt_type, g_config.encrypt_key)) {
            print("[server] Send error during file transfer.", CLR_RED);
            return false;
        }
        sent += n;
        m_bytes_sent += HEADER_SIZE + n;
        if (g_config.verbose && file_size > 0) {
            int pct = (int)(sent * 100 / file_size);
            std::cout << "\r[server] Sending: " << pct << "% ("
                      << fmt_bytes(sent) << "/" << fmt_bytes(file_size) << ")  "
                      << std::flush;
        }
    }

    /* Send the end-of-transfer sentinel. */
    net_send_packet(ci->sock, MSG_FILE_END, SERVER_ID, ci->id,
                    nullptr, 0,
                    g_config.encrypt_type, g_config.encrypt_key);

    std::cout << "\n";
    print("[server] File sent: " + filename + " (" + fmt_bytes(sent) + ")", CLR_GREEN);
    log("FILE_SENT to=" + std::to_string(ci->id) +
        " file=" + filename + " size=" + std::to_string(sent));
    return true;
}

// ============================================================
//  Public File-Send Wrappers
// ============================================================

/**
 * @brief Send a file to a single client identified by numeric ID.
 *
 * Looks up the client in @c m_clients under @c m_clients_mutex, then delegates
 * to @ref send_file_to_client().
 *
 * @param target_id  Numeric ID of the destination client.
 * @param filepath   Path to the file on the server's filesystem.
 * @return           @c true on success, @c false if the client is not found or transfer failed.
 */
bool Server::send_file(uint32_t target_id, const std::string& filepath) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    auto it = m_clients.find(target_id);
    if (it == m_clients.end()) {
        print("[server] Client " + std::to_string(target_id) + " not found.", CLR_RED);
        return false;
    }
    return send_file_to_client(it->second, filepath);
}

/**
 * @brief Send a file to every currently connected client.
 *
 * Iterates the client map under @c m_clients_mutex and calls
 * @ref send_file_to_client() for each entry.  The method continues even if
 * one transfer fails; the return value reflects aggregate success.
 *
 * @param filepath  Path to the file on the server's filesystem.
 * @return          @c true if every individual transfer succeeded, @c false if any failed.
 */
bool Server::send_file_all(const std::string& filepath) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    bool all_ok = true;
    for (auto& kv : m_clients)
        if (!send_file_to_client(kv.second, filepath)) all_ok = false;
    return all_ok;
}

// ============================================================
//  Broadcast and Unicast Text
// ============================================================

/**
 * @brief Transmit a text message to every connected client.
 *
 * Iterates @c m_clients under @c m_clients_mutex and sends a @ref MSG_BROADCAST
 * packet to each.  Also logs and prints the message server-side.
 *
 * @param msg      The text payload to broadcast.
 * @param from_id  Originator ID embedded in the packet header (default @ref SERVER_ID).
 */
void Server::broadcast_text(const std::string& msg, uint32_t from_id) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    for (auto& kv : m_clients) {
        net_send_packet(kv.second->sock, MSG_BROADCAST, from_id, kv.first,
                        msg.c_str(), (uint32_t)msg.size(),
                        g_config.encrypt_type, g_config.encrypt_key);
        m_bytes_sent += HEADER_SIZE + msg.size();
    }
    print("[broadcast] " + msg, CLR_GREEN);
    ++m_msg_count;
}

/**
 * @brief Send a private text message to a single client.
 *
 * Looks up @p target_id in @c m_clients under @c m_clients_mutex and sends
 * a @ref MSG_PRIVATE packet.  Logs an error if the client is not found.
 *
 * @param target_id  Numeric ID of the destination client.
 * @param msg        Text payload.
 * @param from_id    Originator ID in the packet header (default @ref SERVER_ID).
 */
void Server::send_text(uint32_t target_id, const std::string& msg, uint32_t from_id) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    auto it = m_clients.find(target_id);
    if (it == m_clients.end()) {
        print("[server] Client " + std::to_string(target_id) + " not found.", CLR_RED);
        return;
    }
    net_send_packet(it->second->sock, MSG_PRIVATE, from_id, target_id,
                    msg.c_str(), (uint32_t)msg.size(),
                    g_config.encrypt_type, g_config.encrypt_key);
    m_bytes_sent += HEADER_SIZE + msg.size();
    print("[PM->client " + std::to_string(target_id) + "] " + msg, CLR_MAGENTA);
    ++m_msg_count;
}

/**
 * @brief Send an arbitrary typed packet with a text payload to a single client.
 *
 * More flexible than @ref send_text(); the caller supplies the @ref MsgType.
 * Useful for administrative messages that do not fit the standard text types.
 *
 * @param target_id  Numeric ID of the destination client.
 * @param type       @ref MsgType value for the packet header.
 * @param text       Text payload.
 */
void Server::send_raw_msg(uint32_t target_id, uint32_t type, const std::string& text) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    auto it = m_clients.find(target_id);
    if (it == m_clients.end()) return;
    net_send_packet(it->second->sock, type, SERVER_ID, target_id,
                    text.c_str(), (uint32_t)text.size(),
                    g_config.encrypt_type, g_config.encrypt_key);
}

// ============================================================
//  Client List Distribution
// ============================================================

/**
 * @brief Serialize the current client roster and send it to one or all clients.
 *
 * Wire format of the payload:
 * @code
 * [uint32_t count]
 * for each client:
 *   [uint32_t id][uint16_t nick_len][nick bytes…]
 * @endcode
 *
 * When @p target_id is @c 0 the list is broadcast to every connected client;
 * otherwise it is sent only to the client with that ID.
 *
 * @param target_id  Recipient client ID, or @c 0 to send to all clients.
 *
 * @warning Acquires @c m_clients_mutex internally.
 */
void Server::send_client_list(uint32_t target_id) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);

    /* Serialize: [count][id][nick_len][nick]… */
    std::vector<uint8_t> buf;
    uint32_t count = (uint32_t)m_clients.size();
    buf.resize(4);
    memcpy(buf.data(), &count, 4);

    for (auto& kv : m_clients) {
        uint32_t cid = kv.first;
        std::string& nick = kv.second->nickname;
        uint16_t nl = (uint16_t)nick.size();
        size_t off = buf.size();
        buf.resize(off + 4 + 2 + nick.size());
        memcpy(buf.data() + off,     &cid, 4);
        memcpy(buf.data() + off + 4, &nl,  2);
        memcpy(buf.data() + off + 6, nick.c_str(), nick.size());
    }

    if (target_id == 0) {
        /* Broadcast the list to every connected client. */
        for (auto& kv : m_clients) {
            net_send_packet(kv.second->sock, MSG_CLIENT_LIST, SERVER_ID, kv.first,
                            buf.data(), (uint32_t)buf.size(),
                            g_config.encrypt_type, g_config.encrypt_key);
        }
    } else {
        auto it = m_clients.find(target_id);
        if (it != m_clients.end()) {
            net_send_packet(it->second->sock, MSG_CLIENT_LIST, SERVER_ID, target_id,
                            buf.data(), (uint32_t)buf.size(),
                            g_config.encrypt_type, g_config.encrypt_key);
        }
    }
}

// ============================================================
//  Client Removal
// ============================================================

/**
 * @brief Remove a client entry from the internal map and close its socket.
 *
 * Closes the client's socket and erases the entry from @c m_clients.  Note
 * that because the client thread is detached, the @ref ClientInfo memory is
 * not freed here — it leaks in the current implementation.  A production system
 * would require a garbage-collection mechanism.
 *
 * @param id  Numeric ID of the client to remove.
 *
 * @warning Acquires @c m_clients_mutex internally.  Do not call while already
 *          holding this lock.
 */
void Server::remove_client(uint32_t id) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    auto it = m_clients.find(id);
    if (it != m_clients.end()) {
        sock_close(it->second->sock);
        /* NOTE: memory is intentionally not freed here because the client
           thread (which is detached) may still be executing.  A proper
           implementation would use a delayed GC or shared_ptr. */
        m_clients.erase(it);
    }
}

// ============================================================
//  Kick
// ============================================================

/**
 * @brief Forcefully disconnect a client with an optional reason.
 *
 * Sends @ref MSG_KICK with a human-readable message, marks the client as no
 * longer alive, and closes its socket.  The client's receive thread will
 * subsequently detect the closed socket and exit.
 *
 * @param id      Numeric ID of the client to kick.
 * @param reason  Optional human-readable reason appended to the kick message.
 */
void Server::kick_client(uint32_t id, const std::string& reason) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    auto it = m_clients.find(id);
    if (it == m_clients.end()) {
        print("[server] Cannot kick: client " + std::to_string(id) + " not found.", CLR_RED);
        return;
    }
    std::string msg = "You have been kicked. " + reason;
    net_send_packet(it->second->sock, MSG_KICK, SERVER_ID, id,
                    msg.c_str(), (uint32_t)msg.size(),
                    g_config.encrypt_type, g_config.encrypt_key);
    it->second->alive = false;
    sock_close(it->second->sock);
    print("[server] Kicked client " + std::to_string(id) +
          " (" + it->second->nickname + ") " + (reason.empty() ? "" : "| " + reason),
          CLR_YELLOW);
}

// ============================================================
//  Diagnostics
// ============================================================

/**
 * @brief Print a formatted table of connected clients to @c stdout.
 *
 * Acquires @c m_clients_mutex, then writes a row per client containing their
 * ID, nickname (truncated to 18 characters), IP address, and connection time.
 * If no clients are connected, a short notice is printed instead.
 */
void Server::list_clients() {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    if (m_clients.empty()) {
        std::cout << "[server] No clients connected.\n";
        return;
    }
    std::cout << clr(CLR_BOLD) << "  ID  | Nickname           | IP              | Connected\n"
              << "------+--------------------+-----------------+------------------------\n"
              << clr(CLR_RESET);
    for (auto& kv : m_clients) {
        char ts[32];
        time_t t = kv.second->connected_at;
        strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&t));
        char row[128];
        snprintf(row, sizeof(row), "  %-4u | %-18s | %-15s | %s",
                 kv.first,
                 kv.second->nickname.substr(0, 18).c_str(),
                 kv.second->ip.c_str(),
                 ts);
        std::cout << row << "\n";
    }
    std::cout << "  Total: " << m_clients.size() << " client(s)\n";
}

/**
 * @brief Print a summary of server statistics to @c stdout.
 *
 * Reports: active client count, total messages routed, bytes sent, bytes
 * received, and the currently active encryption type.
 */
void Server::print_stats() {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    std::cout << clr(CLR_BOLD) << "=== Server Stats ===\n" << clr(CLR_RESET)
              << "  Clients online : " << m_clients.size() << "\n"
              << "  Messages routed: " << m_msg_count << "\n"
              << "  Bytes sent     : " << fmt_bytes(m_bytes_sent) << "\n"
              << "  Bytes received : " << fmt_bytes(m_bytes_recv) << "\n"
              << "  Encryption     : " << encrypt_type_name(g_config.encrypt_type) << "\n";
}

/**
 * @brief Look up a client's numeric ID by their current nickname.
 *
 * Performs a linear scan of @c m_clients under @c m_clients_mutex.
 * Comparison is case-sensitive and exact.
 *
 * @param nick  Nickname to search for.
 * @return      The matching client's numeric ID, or @c 0 if not found.
 */
uint32_t Server::find_client_by_nick(const std::string& nick) {
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    for (auto& kv : m_clients)
        if (kv.second->nickname == nick) return kv.first;
    return 0;
}

// ============================================================
//  Keepalive Loop
// ============================================================

/**
 * @brief Periodically ping all connected clients to detect stale connections.
 *
 * Runs on @c m_keepalive_thread.  Sleeps @c g_config.ping_interval seconds
 * between iterations (implemented as a 1-second polling loop so that
 * @c m_running changes are detected promptly).  On each wakeup, sends a
 * @ref MSG_PING to every connected client.
 */
void Server::keepalive_loop() {
    while (m_running) {
        /* Sleep in 1-second increments to remain responsive to stop(). */
        for (int i = 0; i < g_config.ping_interval && m_running; ++i) {
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
        }
        if (!m_running) break;

        std::lock_guard<std::mutex> lk(m_clients_mutex);
        for (auto& kv : m_clients) {
            net_send_packet(kv.second->sock, MSG_PING, SERVER_ID, kv.first,
                            nullptr, 0,
                            g_config.encrypt_type, g_config.encrypt_key);
        }
    }
}

// ============================================================
//  Packet Routing Helper
// ============================================================

/**
 * @brief Route a packet to one or all clients, optionally excluding a sender.
 *
 * If @c hdr.target_id equals @ref BROADCAST_ID, the packet is forwarded to
 * every client whose ID is not @p exclude_id.  Otherwise it is sent only to
 * the client whose ID matches @c hdr.target_id.
 *
 * @param hdr        Header of the packet to route; @c msg_type, @c sender_id,
 *                   and @c target_id fields are reused in the forwarded copies.
 * @param payload    Payload bytes to attach to each forwarded packet.
 * @param exclude_id Client ID that should not receive the packet (defaults to
 *                   @c 0xFFFFFFFE, an ID that is never assigned).
 */
void Server::distribute_packet(const PacketHeader& hdr,
                                const std::vector<uint8_t>& payload,
                                uint32_t exclude_id)
{
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    if (hdr.target_id == BROADCAST_ID) {
        for (auto& kv : m_clients) {
            if (kv.first == exclude_id) continue;
            net_send_packet(kv.second->sock, hdr.msg_type, hdr.sender_id, kv.first,
                            payload.data(), (uint32_t)payload.size(),
                            g_config.encrypt_type, g_config.encrypt_key);
        }
    } else {
        auto it = m_clients.find(hdr.target_id);
        if (it != m_clients.end()) {
            net_send_packet(it->second->sock, hdr.msg_type, hdr.sender_id, it->first,
                            payload.data(), (uint32_t)payload.size(),
                            g_config.encrypt_type, g_config.encrypt_key);
        }
    }
}

// ============================================================
//  CLI Help
// ============================================================

/**
 * @brief Print the server operator help text to @c stdout.
 *
 * Lists every supported CLI command with a short usage description.
 * Called on startup and in response to the @c /help or @c /? command.
 */
static void print_server_help() {
    std::cout <<
        "\n" << "\033[1m" << "=== SafeSocket Server Commands ===" << "\033[0m" << "\n"
        "  /list                     - List all connected clients\n"
        "  /msg <id> <text>          - Send private message to client by ID\n"
        "  /msgn <nick> <text>       - Send private message to client by nickname\n"
        "  /broadcast <text>         - Broadcast message to all clients\n"
        "  /sendfile <id> <path>     - Send file to specific client\n"
        "  /sendfileall <path>       - Send file to ALL clients\n"
        "  /kick <id> [reason]       - Kick a client\n"
        "  /kickn <nick> [reason]    - Kick a client by nickname\n"
        "  /stats                    - Show server statistics\n"
        "  /config                   - Show current configuration\n"
        "  /set <key> <value>        - Change configuration at runtime\n"
        "  /setconfig <key> <value>  - Same as /set\n"
        "  /saveconfig [path]        - Save config to file\n"
        "  /loadconfig [path]        - Reload config from file\n"
        "  /confighelp               - List all config keys\n"
        "  /quit  /stop              - Stop server and exit\n"
        "  /help  /?                 - Show this help\n\n";
}

// ============================================================
//  Interactive CLI
// ============================================================

/**
 * @brief Block the calling thread running the operator command-line interface.
 *
 * Reads commands from @c stdin in a loop until the operator types @c /quit,
 * @c /stop, or @c /exit, or until @c stdin reaches EOF.  Each iteration:
 *  1. Prints the @c "[SERVER]> " prompt.
 *  2. Reads a line with @c std::getline().
 *  3. Trims whitespace and skips blank input.
 *  4. Parses the leading command token (normalised to lowercase).
 *  5. Dispatches to the appropriate @ref Server method or @ref Config operation.
 *
 * Supported commands: @c /list, @c /msg, @c /msgn, @c /broadcast,
 * @c /kick, @c /kickn, @c /sendfile, @c /sendfileall, @c /stats, @c /config,
 * @c /confighelp, @c /set, @c /setconfig, @c /saveconfig, @c /loadconfig,
 * @c /quit (@c /stop, @c /exit), @c /help (@c /?).
 */
void Server::run_cli() {
    print_server_help();
    std::string line;

    while (m_running) {
        std::cout << clr(CLR_BOLD) << clr(CLR_GREEN)
                  << "[SERVER]> " << clr(CLR_RESET);
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;  /* EOF — exit cleanly. */

        /* Strip trailing carriage returns and newlines from Windows line endings. */
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) continue;

        /* Tokenise: extract the command verb and leave the rest in the stream. */
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        /* Normalise the command to lowercase for case-insensitive matching. */
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "/quit" || cmd == "/stop" || cmd == "/exit") {
            stop();
            break;
        }
        else if (cmd == "/help" || cmd == "/?") {
            print_server_help();
        }
        else if (cmd == "/list") {
            list_clients();
        }
        else if (cmd == "/stats") {
            print_stats();
        }
        else if (cmd == "/config") {
            g_config.print();
        }
        else if (cmd == "/confighelp") {
            g_config.print_help();
        }
        else if (cmd == "/set" || cmd == "/setconfig") {
            std::string key, value;
            iss >> key;
            std::getline(iss, value);
            /* Strip leading space between key and value. */
            while (!value.empty() && value[0] == ' ') value = value.substr(1);
            if (key.empty()) { std::cout << "Usage: /set <key> <value>\n"; continue; }
            g_config.set(key, value);
            std::cout << "[config] " << key << " = " << g_config.get(key) << "\n";
        }
        else if (cmd == "/saveconfig") {
            std::string path;
            iss >> path;
            if (path.empty()) path = "safesocket.conf";
            if (g_config.save(path))
                std::cout << "[config] Saved to " << path << "\n";
            else
                std::cout << "[config] Save failed!\n";
        }
        else if (cmd == "/loadconfig") {
            std::string path;
            iss >> path;
            if (path.empty()) path = "safesocket.conf";
            if (g_config.load(path))
                std::cout << "[config] Loaded from " << path << "\n";
            else
                std::cout << "[config] Load failed!\n";
        }
        else if (cmd == "/broadcast") {
            std::string msg;
            std::getline(iss, msg);
            while (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            if (msg.empty()) { std::cout << "Usage: /broadcast <message>\n"; continue; }
            broadcast_text("[SERVER] " + msg, SERVER_ID);
        }
        else if (cmd == "/msg") {
            uint32_t id; std::string msg;
            iss >> id;
            std::getline(iss, msg);
            while (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            if (msg.empty()) { std::cout << "Usage: /msg <id> <message>\n"; continue; }
            send_text(id, "[SERVER] " + msg, SERVER_ID);
        }
        else if (cmd == "/msgn") {
            std::string nick, msg;
            iss >> nick;
            std::getline(iss, msg);
            while (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            uint32_t id = find_client_by_nick(nick);
            if (id == 0) { std::cout << "Client '" << nick << "' not found.\n"; continue; }
            send_text(id, "[SERVER] " + msg, SERVER_ID);
        }
        else if (cmd == "/kick") {
            uint32_t id; std::string reason;
            iss >> id;
            std::getline(iss, reason);
            while (!reason.empty() && reason[0] == ' ') reason = reason.substr(1);
            kick_client(id, reason);
        }
        else if (cmd == "/kickn") {
            std::string nick, reason;
            iss >> nick;
            std::getline(iss, reason);
            while (!reason.empty() && reason[0] == ' ') reason = reason.substr(1);
            uint32_t id = find_client_by_nick(nick);
            if (id == 0) { std::cout << "Client '" << nick << "' not found.\n"; continue; }
            kick_client(id, reason);
        }
        else if (cmd == "/sendfile") {
            uint32_t id; std::string path;
            iss >> id;
            std::getline(iss, path);
            while (!path.empty() && path[0] == ' ') path = path.substr(1);
            if (path.empty()) { std::cout << "Usage: /sendfile <id> <filepath>\n"; continue; }
            send_file(id, path);
        }
        else if (cmd == "/sendfileall") {
            std::string path;
            std::getline(iss, path);
            while (!path.empty() && path[0] == ' ') path = path.substr(1);
            if (path.empty()) { std::cout << "Usage: /sendfileall <filepath>\n"; continue; }
            send_file_all(path);
        }
        else {
            std::cout << "[server] Unknown command: " << cmd
                      << " (type /help for commands)\n";
        }
    }
}
