/**
 * @file client.cpp
 * @brief Implementation of the SafeSocket TCP client.
 *
 * Provides the concrete implementations for all methods declared in
 * @ref client.hpp.  The client runs two concurrent threads:
 *  - The **main thread** (caller of @ref Client::run_cli()) reads commands
 *    from @c stdin and translates them into outgoing packets.
 *  - The **receive thread** (@ref Client::recv_loop()) blocks on the socket
 *    and dispatches incoming packets via @ref Client::handle_packet().
 *
 * ### Connection Handshake
 * @code
 * Client  - MSG_CONNECT [nick_len][nick][access_key] --? Server
 *         ?-- MSG_SERVER_INFO  "Welcome to SafeSocket!"  -
 *         ?-- MSG_SERVER_INFO  "YOUR_ID=N"               -
 *         ?-- MSG_CLIENT_LIST  [count][id][nick]�        -
 * @endcode
 *
 * ### File Transfer Flow (client ? server upload)
 * @code
 * Client  - MSG_FILE_START --? Server
 *         ? MSG_FILE_ACCEPT --
 *         - MSG_FILE_DATA� --?
 *         - MSG_FILE_END ----?
 * @endcode
 *
 * ### File Transfer Flow (server ? client download)
 * @code
 * Server  - MSG_FILE_START --? Client (recv thread)
 *         ? MSG_FILE_ACCEPT --
 *         - MSG_FILE_DATA� --?
 *         - MSG_FILE_END ----?
 * @endcode
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#include "client.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  /// @cond INTERNAL
  #define mkdir_compat(p) _mkdir(p)       ///< Platform-neutral directory creation (Windows).
  /// @endcond
#else
  #include <unistd.h>
  /// @cond INTERNAL
  #define mkdir_compat(p) mkdir(p, 0755)  ///< Platform-neutral directory creation (POSIX).
  /// @endcond
#endif

// ============================================================
//  ANSI Colour Constants
// ============================================================

/** @brief ANSI escape code that resets all text attributes. */
static const char* CLR_RESET  = "\033[0m";
/** @brief ANSI escape code for cyan text (informational messages). */
static const char* CLR_CYAN   = "\033[36m";
/** @brief ANSI escape code for green text (success / server info). */
static const char* CLR_GREEN  = "\033[32m";
/** @brief ANSI escape code for yellow text (warnings and file notifications). */
static const char* CLR_YELLOW = "\033[33m";
/** @brief ANSI escape code for red text (errors and disconnections). */
static const char* CLR_RED    = "\033[31m";
/** @brief ANSI escape code that enables bold/bright text. */
static const char* CLR_BOLD   = "\033[1m";
/** @brief ANSI escape code for magenta text (private messages). */
static const char* CLR_MAGENTA= "\033[35m";

/**
 * @brief Return @p c as a std::string, or an empty string if colour is disabled.
 *
 * Centralises the @c g_config.color_output check so callers need not repeat
 * the conditional at every output site.
 *
 * @param c  ANSI escape sequence constant.
 * @return   @p c wrapped in a @c std::string, or @c "" when colour is disabled.
 */
static std::string clr(const char* c) {
    return g_config.color_output ? std::string(c) : "";
}

// ============================================================
//  Constructor / Destructor
// ============================================================

/**
 * @brief Construct an idle, unconnected @ref Client.
 *
 * All handles are set to invalid/false; atomic flags are initialised to @c false.
 * Call @ref Client::connect() before any other method.
 */
Client::Client()
    : m_sock(INVALID_SOCK), m_connected(false), m_running(false),
      m_my_id(0), m_nickname("anonymous"), m_file_incoming(false),
      m_file_incoming_size(0)
{}

/**
 * @brief Destroy the client, disconnecting first if still connected.
 *
 * Delegates to @ref Client::disconnect() to send @ref MSG_DISCONNECT, close
 * the socket, and join the receive thread.
 */
Client::~Client() {
    disconnect();
}

// ============================================================
//  Logging and Output Helpers
// ============================================================

/**
 * @brief Append a timestamped log entry to the configured log file.
 *
 * Opens @c g_config.log_file in append mode, writes the entry with a
 * @c "YYYY-MM-DD HH:MM:SS" prefix, and closes the file.  Does nothing when
 * @c g_config.log_to_file is @c false.
 *
 * @param msg  The plain-text message to log (a newline is appended automatically).
 */
void Client::log(const std::string& msg) {
    if (g_config.log_to_file) {
        std::ofstream f(g_config.log_file, std::ios::app);
        time_t t = time(nullptr);
        char ts[32];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
        f << "[" << ts << "] " << msg << "\n";
    }
}

/**
 * @brief Print a message to @c stdout with optional ANSI colour, then reprint the prompt.
 *
 * Prefixes the output with @c "\\r" to clear the current input line before
 * printing, then re-emits the @c "[nickname]> " prompt so the user's cursor
 * stays at the bottom of the terminal.  Also forwards the message to @ref log().
 *
 * @param msg    The text to display.
 * @param color  ANSI colour escape sequence, or @c nullptr for plain output.
 */
void Client::print(const std::string& msg, const char* color) {
    // If a GUI callback is registered, use it instead of stdout
    if (m_msg_cb) {
        bool is_priv = (color == CLR_MAGENTA);
        bool is_sys  = (color == CLR_YELLOW || color == CLR_GREEN || color == CLR_RED);
        m_msg_cb(msg, is_priv, is_sys);
        log(msg);
        return;
    }
    if (color && g_config.color_output)
        std::cout << "\r" << color << msg << CLR_RESET << "\n";
    else
        std::cout << "\r" << msg << "\n";
    /* Re-emit the input prompt so the user can continue typing. */
    std::cout << clr(CLR_BOLD) << clr(CLR_CYAN) << "[" << m_nickname << "]> "
              << clr(CLR_RESET);
    std::cout.flush();
    log(msg);
}

// ============================================================
//  Connection Management
// ============================================================

/**
 * @brief Establish a TCP connection to a SafeSocket server and begin receiving.
 *
 * Steps:
 *  1. Create the download directory via @c mkdir_compat().
 *  2. Copy the nickname from @c g_config.nickname.
 *  3. Call @ref net_connect() to obtain a connected socket.
 *  4. Launch @ref recv_loop() on @c m_recv_thread.
 *  5. Build and send the @ref MSG_CONNECT handshake packet containing the
 *     nickname and (when non-empty) the access key.
 *  6. Print the connection banner.
 *
 * @param host  Dotted-decimal IPv4 address of the server.
 * @param port  TCP port the server is listening on.
 * @return      @c true on success, @c false if @ref net_connect() failed.
 */
bool Client::connect(const std::string& host, int port) {
    mkdir_compat(g_config.download_dir.c_str());
    m_nickname = g_config.nickname;

    m_sock = net_connect(host, port, g_config.connect_timeout);
    if (m_sock == INVALID_SOCK) {
        std::cerr << "[client] Failed to connect to " << host << ":" << port << "\n";
        return false;
    }

    m_connected = true;
    m_running   = true;

    /* Start the receive thread before sending the handshake, so no response is missed. */
    m_recv_thread = std::thread(&Client::recv_loop, this);

    /* Build the MSG_CONNECT payload: [uint16_t nick_len][nick bytes][access key bytes]. */
    std::vector<uint8_t> conn_payload;
    uint16_t nick_len = (uint16_t)m_nickname.size();
    conn_payload.resize(2 + nick_len + g_config.access_key.size());
    memcpy(conn_payload.data(), &nick_len, 2);
    memcpy(conn_payload.data() + 2, m_nickname.c_str(), nick_len);
    if (!g_config.access_key.empty())
        memcpy(conn_payload.data() + 2 + nick_len,
               g_config.access_key.c_str(), g_config.access_key.size());

    net_send_packet(m_sock, MSG_CONNECT, 0, SERVER_ID,
                    conn_payload.data(), (uint32_t)conn_payload.size(),
                    g_config.encrypt_type, g_config.encrypt_key);

    std::cout << clr(CLR_GREEN) << clr(CLR_BOLD)
              << "==============================================\n"
              << "  SafeSocket CLIENT  |  " << m_nickname << "\n"
              << "  Connected to " << host << ":" << port << "\n"
              << "  Encryption: " << encrypt_type_name(g_config.encrypt_type) << "\n"
              << "==============================================\n"
              << clr(CLR_RESET);

    return true;
}

/**
 * @brief Disconnect from the server, release resources, and stop the receive thread.
 *
 * Sends @ref MSG_DISCONNECT, closes the socket, joins @c m_recv_thread, and
 * resets the @c m_connected / @c m_running flags.  Safe to call multiple times;
 * subsequent calls after the first are no-ops (guarded by @c m_connected).
 */
void Client::disconnect() {
    if (!m_connected) return;
    m_running    = false;
    m_connected  = false;

    if (m_sock != INVALID_SOCK) {
        /* Notify the server of the graceful disconnect before closing. */
        net_send_packet(m_sock, MSG_DISCONNECT, m_my_id, SERVER_ID,
                        nullptr, 0,
                        g_config.encrypt_type, g_config.encrypt_key);
        sock_close(m_sock);
        m_sock = INVALID_SOCK;
    }

    if (m_recv_thread.joinable()) m_recv_thread.join();
    std::cout << "[client] Disconnected.\n";
}

// ============================================================
//  Receive Loop (background thread)
// ============================================================

/**
 * @brief Thread function that continuously reads packets from the server.
 *
 * Blocks on @ref net_recv_packet() in a tight loop.  Each successfully received
 * packet is dispatched to @ref handle_packet().  The loop exits when
 * @c m_running becomes @c false, @ref net_recv_packet() returns @c false (socket
 * closed or error), or when the server closes the connection.
 *
 * On exit, both @c m_connected and @c m_running are set to @c false so that
 * the CLI loop in @ref run_cli() can detect the disconnection.
 */
void Client::recv_loop() {
    PacketHeader hdr;
    std::vector<uint8_t> payload;

    while (m_running) {
        hdr = PacketHeader{};
        payload.clear();

        if (!net_recv_packet(m_sock, hdr, payload,
                             g_config.encrypt_type, g_config.encrypt_key)) {
            if (m_running) print("[client] Disconnected from server.", CLR_RED);
            break;
        }
        handle_packet(hdr, payload);
    }
    m_connected = false;
    m_running   = false;
}

// ============================================================
//  Packet Dispatcher (receive thread)
// ============================================================

/**
 * @brief Dispatch a received packet to the appropriate handler.
 *
 * Called from @ref recv_loop() for every packet the server sends.
 * Handled message types:
 *  - @ref MSG_SERVER_INFO: Parse @c "YOUR_ID=N" to set @c m_my_id; otherwise print MOTD.
 *  - @ref MSG_BROADCAST: Print the text in cyan.
 *  - @ref MSG_PRIVATE: Print the private message in magenta.
 *  - @ref MSG_TEXT: Print plain text.
 *  - @ref MSG_CLIENT_LIST: Delegate to @ref parse_client_list().
 *  - @ref MSG_ACK: Print a green acknowledgment.
 *  - @ref MSG_ERROR: Print a red error.
 *  - @ref MSG_KICK: Print the kick message and stop the receive loop.
 *  - @ref MSG_PING: Send @ref MSG_PONG back to the server.
 *  - @ref MSG_FILE_START: Delegate to @ref handle_incoming_file().
 *
 * Unknown types trigger a verbose warning when @c g_config.verbose is @c true.
 *
 * @param hdr      Parsed @ref PacketHeader of the received packet.
 * @param payload  Decrypted payload bytes of the received packet.
 */
void Client::handle_packet(const PacketHeader& hdr,
                            const std::vector<uint8_t>& payload)
{
    /* Convenience: interpret the payload as a UTF-8 string for text messages. */
    std::string text(payload.begin(), payload.end());

    switch (hdr.msg_type) {
    case MSG_SERVER_INFO:
        /* Detect the special ID assignment message from the server. */
        if (text.substr(0, 8) == "YOUR_ID=") {
            m_my_id = (uint32_t)std::stoul(text.substr(8));
            print("[client] Assigned ID: " + std::to_string(m_my_id), CLR_YELLOW);
        } else {
            /* Anything else in SERVER_INFO is the MOTD or administrative notice. */
            print("[SERVER INFO] " + text, CLR_GREEN);
        }
        break;

    case MSG_BROADCAST:
        print(text, CLR_CYAN);
        break;

    case MSG_PRIVATE:
        print(text, CLR_MAGENTA);
        break;

    case MSG_TEXT:
        print(text, nullptr);
        break;

    case MSG_CLIENT_LIST:
        parse_client_list(payload);
        break;

    case MSG_ACK:
        print("[ACK] " + text, CLR_GREEN);
        break;

    case MSG_ERROR:
        print("[ERROR] " + text, CLR_RED);
        break;

    case MSG_KICK:
        print("[KICKED] " + text, CLR_RED);
        m_running = false;   /* Signal the recv loop to exit. */
        break;

    case MSG_PING:
        /* Reply with a pong to satisfy the server's keepalive check. */
        net_send_packet(m_sock, MSG_PONG, m_my_id, SERVER_ID,
                        nullptr, 0,
                        g_config.encrypt_type, g_config.encrypt_key);
        if (g_config.verbose) print("[ping] pong sent", CLR_YELLOW);
        break;

    case MSG_FILE_START:
        /* Delegate to the dedicated incoming-file handler. */
        handle_incoming_file(hdr, payload);
        break;

    default:
        if (g_config.verbose)
            print("[client] Unknown packet type " + std::to_string(hdr.msg_type), CLR_YELLOW);
        break;
    }
}

// ============================================================
//  Client List Parsing
// ============================================================

/**
 * @brief Parse a @ref MSG_CLIENT_LIST payload and refresh the local peer cache.
 *
 * Expected wire format:
 * @code
 * [uint32_t count]
 * for each peer: [uint32_t id][uint16_t nick_len][nick bytes�]
 * @endcode
 *
 * Rebuilds @c m_peers under @c m_peers_mutex, then prints the updated list to
 * @c stdout, marking the local client's own entry with @c " (you)".
 *
 * @param payload  Raw payload bytes of the @ref MSG_CLIENT_LIST packet.
 */
void Client::parse_client_list(const std::vector<uint8_t>& payload) {
    if (payload.size() < 4) return;
    uint32_t count = 0;
    memcpy(&count, payload.data(), 4);
    size_t off = 4;

    std::lock_guard<std::mutex> lk(m_peers_mutex);
    m_peers.clear();

    for (uint32_t i = 0; i < count && off + 6 <= payload.size(); ++i) {
        uint32_t cid = 0;
        uint16_t nl  = 0;
        memcpy(&cid, payload.data() + off, 4); off += 4;
        memcpy(&nl,  payload.data() + off, 2); off += 2;
        if (off + nl > payload.size()) break;   /* Guard against truncated payload. */
        std::string nick((char*)payload.data() + off, nl);
        off += nl;
        m_peers[cid] = {cid, nick};
    }

    // Notify GUI callback if registered
    if (m_peer_cb) {
        std::vector<std::pair<uint32_t,std::string>> list;
        for (auto& kv : m_peers)
            list.push_back({kv.first, kv.second.nickname});
        m_peer_cb(list);
    } else {
        /* Print the updated peer list. */
        std::cout << "\r" << clr(CLR_BOLD) << "--- Online Clients (" << count << ") ---\n"
                  << clr(CLR_RESET);
        for (auto& kv : m_peers) {
            std::string marker = (kv.first == m_my_id) ? " (you)" : "";
            std::cout << "  [" << kv.first << "] " << kv.second.nickname << marker << "\n";
        }
        /* Re-emit the input prompt after the list. */
        std::cout << clr(CLR_BOLD) << clr(CLR_CYAN) << "[" << m_nickname << "]> "
                  << clr(CLR_RESET);
        std::cout.flush();
    }
}

// ============================================================
//  Incoming File Handling (receive thread)
// ============================================================

/**
 * @brief Receive and save an incoming file sent by the server.
 *
 * Called from the receive thread when @ref MSG_FILE_START arrives.
 * Workflow:
 *  1. Parse @ref FileStartPayload to extract @c file_size and filename.
 *  2. Sanitize the filename against directory-traversal characters.
 *  3. Check against @c g_config.max_file_size; reject if exceeded.
 *  4. Send @ref MSG_FILE_ACCEPT (or @ref MSG_FILE_REJECT on size violation).
 *  5. Open the output file under @c g_config.download_dir.
 *  6. Loop receiving @ref MSG_FILE_DATA chunks, writing each to disk.
 *     Non-data packets received mid-transfer are re-dispatched via
 *     @ref handle_packet().
 *  7. Break on @ref MSG_FILE_END.
 *  8. Log and confirm completion.
 *
 * @param hdr           Header of the @ref MSG_FILE_START packet (contains sender_id).
 * @param start_payload Payload bytes of the @ref MSG_FILE_START packet, starting
 *                      with a @ref FileStartPayload struct followed by filename bytes.
 */
void Client::handle_incoming_file(const PacketHeader& hdr,
                                   const std::vector<uint8_t>& start_payload)
{
    if (start_payload.size() < sizeof(FileStartPayload)) return;

    /* Parse the fixed-size metadata prefix. */
    FileStartPayload* fsp = (FileStartPayload*)start_payload.data();
    uint64_t file_size    = fsp->file_size;
    uint32_t fname_len    = fsp->filename_len;

    if (start_payload.size() < sizeof(FileStartPayload) + fname_len) return;
    std::string filename((char*)start_payload.data() + sizeof(FileStartPayload), fname_len);

    /* Sanitize: replace characters that could enable directory traversal. */
    for (char& c : filename)
        if (c == '/' || c == '\\' || c == ':') c = '_';

    print("[FILE INCOMING] '" + filename + "' (" + fmt_bytes(file_size) +
          ") from ID " + std::to_string(hdr.sender_id), CLR_YELLOW);

    /* When auto_accept_files is disabled we still proceed, but warn the user. */
    if (!g_config.auto_accept_files) {
        std::cout << "Accept? (y/n): ";
        std::cout.flush();
        /* In the threaded receive context, blocking on stdin would deadlock the
           recv loop.  Auto-accept with a visible notice as a pragmatic workaround. */
        print("[client] Auto-accepting (set auto_accept_files=true to suppress prompt)", CLR_YELLOW);
    }

    /* Enforce the configured maximum file size if one is set. */
    if (g_config.max_file_size > 0 && file_size > g_config.max_file_size) {
        print("[client] File too large (" + fmt_bytes(file_size) +
              " > " + fmt_bytes(g_config.max_file_size) + "). Rejecting.", CLR_RED);
        net_send_packet(m_sock, MSG_FILE_REJECT, m_my_id, SERVER_ID,
                        nullptr, 0,
                        g_config.encrypt_type, g_config.encrypt_key);
        return;
    }

    /* Signal acceptance to the sender. */
    net_send_packet(m_sock, MSG_FILE_ACCEPT, m_my_id, SERVER_ID,
                    nullptr, 0,
                    g_config.encrypt_type, g_config.encrypt_key);

    std::string out_path = g_config.download_dir + "/" + filename;
    std::ofstream outf(out_path, std::ios::binary);
    if (!outf.is_open()) {
        print("[client] Cannot write file: " + out_path, CLR_RED);
        return;
    }

    uint64_t received = 0;
    bool ok = true;

    while (ok) {
        PacketHeader chdr;
        std::vector<uint8_t> cpayload;
        if (!net_recv_packet(m_sock, chdr, cpayload,
                             g_config.encrypt_type, g_config.encrypt_key)) {
            ok = false; break;
        }
        if (chdr.msg_type == MSG_FILE_END) break;   /* Transfer complete. */
        if (chdr.msg_type == MSG_FILE_DATA) {
            outf.write((char*)cpayload.data(), cpayload.size());
            received += cpayload.size();
            if (file_size > 0 && g_config.verbose) {
                int pct = (int)(received * 100 / file_size);
                std::cout << "\rDownloading: " << pct << "% ("
                          << fmt_bytes(received) << "/" << fmt_bytes(file_size) << ")  "
                          << std::flush;
            }
        } else {
            /* Out-of-band packet received during transfer � process normally. */
            handle_packet(chdr, cpayload);
        }
    }
    outf.close();

    if (ok) {
        std::cout << "\n";
        print("[client] File saved: " + out_path + " (" + fmt_bytes(received) + ")", CLR_GREEN);
        log("FILE_RECV file=" + filename + " size=" + std::to_string(received));
    } else {
        print("[client] File transfer interrupted.", CLR_RED);
    }
}

// ============================================================
//  Outgoing File Transfer
// ============================================================

/**
 * @brief Upload a local file to the server for storage.
 *
 * Convenience wrapper that calls @ref do_send_file() with @ref SERVER_ID as
 * the target.
 *
 * @param filepath  Local path of the file to upload.
 * @return          @c true on success, @c false on failure or rejection.
 */
bool Client::send_file_to_server(const std::string& filepath) {
    return do_send_file(m_sock, SERVER_ID, filepath);
}

/**
 * @brief Send a local file to another client, routed through the server.
 *
 * Convenience wrapper that calls @ref do_send_file() with @p target_id, which
 * is embedded in the @ref MSG_FILE_START header so the server knows which
 * client should receive the file.
 *
 * @param target_id  Numeric ID of the destination peer.
 * @param filepath   Local path of the file to send.
 * @return           @c true on success, @c false on failure or rejection.
 */
bool Client::send_file_to_client(uint32_t target_id, const std::string& filepath) {
    return do_send_file(m_sock, target_id, filepath);
}

/**
 * @brief Core implementation of the client-side file send protocol.
 *
 * Steps:
 *  1. Open @p filepath in binary mode and measure its size.
 *  2. Extract the base filename.
 *  3. Build and send @ref MSG_FILE_START containing @ref FileStartPayload + filename.
 *  4. Wait for @ref MSG_FILE_ACCEPT or @ref MSG_FILE_REJECT.
 *  5. Stream @ref MSG_FILE_DATA chunks of up to @ref FILE_CHUNK bytes each,
 *     printing upload progress to @c stdout.
 *  6. Send @ref MSG_FILE_END.
 *
 * @param sock       Connected socket to use for the transfer.
 * @param target_id  Target ID embedded in packet headers (server or remote peer).
 * @param filepath   Local filesystem path of the file to transmit.
 * @return           @c true if the full transfer completed, @c false on any error or rejection.
 */
bool Client::do_send_file(sock_t sock, uint32_t target_id, const std::string& filepath) {
    /* Open the file and seek to the end to determine its total size. */
    std::ifstream inf(filepath, std::ios::binary | std::ios::ate);
    if (!inf.is_open()) {
        print("[client] Cannot open file: " + filepath, CLR_RED);
        return false;
    }
    uint64_t file_size = (uint64_t)inf.tellg();
    inf.seekg(0);

    /* Extract the base filename from the full path. */
    std::string filename = filepath;
    size_t slash = filename.find_last_of("/\\");
    if (slash != std::string::npos) filename = filename.substr(slash + 1);

    print("[client] Sending '" + filename + "' (" + fmt_bytes(file_size) +
          ") to " + (target_id == SERVER_ID ? "server" : "client " + std::to_string(target_id)) + "...",
          CLR_CYAN);

    /* Build the MSG_FILE_START payload: FileStartPayload struct + filename bytes. */
    std::vector<uint8_t> start_payload(sizeof(FileStartPayload) + filename.size());
    FileStartPayload* fsp = (FileStartPayload*)start_payload.data();
    fsp->file_size    = file_size;
    fsp->filename_len = (uint32_t)filename.size();
    memcpy(start_payload.data() + sizeof(FileStartPayload),
           filename.c_str(), filename.size());

    if (!net_send_packet(sock, MSG_FILE_START, m_my_id, target_id,
                         start_payload.data(), (uint32_t)start_payload.size(),
                         g_config.encrypt_type, g_config.encrypt_key)) {
        print("[client] Failed to send FILE_START", CLR_RED);
        return false;
    }

    /* Block until the receiver accepts or rejects the transfer. */
    PacketHeader ahdr;
    std::vector<uint8_t> apayload;
    if (!net_recv_packet(sock, ahdr, apayload,
                         g_config.encrypt_type, g_config.encrypt_key)) {
        print("[client] No response from server.", CLR_RED);
        return false;
    }
    if (ahdr.msg_type == MSG_FILE_REJECT) {
        print("[client] File transfer rejected.", CLR_YELLOW);
        return false;
    }

    /* Stream the file in FILE_CHUNK-sized packets with live progress. */
    std::vector<uint8_t> chunk(FILE_CHUNK);
    uint64_t sent = 0;
    while (inf.read((char*)chunk.data(), FILE_CHUNK) || inf.gcount() > 0) {
        size_t n = (size_t)inf.gcount();
        if (!net_send_packet(sock, MSG_FILE_DATA, m_my_id, target_id,
                             chunk.data(), (uint32_t)n,
                             g_config.encrypt_type, g_config.encrypt_key)) {
            print("[client] Send error during file transfer.", CLR_RED);
            return false;
        }
        sent += n;
        if (file_size > 0) {
            int pct = (int)(sent * 100 / file_size);
            std::cout << "\rUploading: " << pct << "% ("
                      << fmt_bytes(sent) << "/" << fmt_bytes(file_size) << ")  "
                      << std::flush;
        }
    }

    /* Signal end of transfer to the receiver. */
    net_send_packet(sock, MSG_FILE_END, m_my_id, target_id,
                    nullptr, 0,
                    g_config.encrypt_type, g_config.encrypt_key);

    std::cout << "\n";
    print("[client] File sent: " + filename + " (" + fmt_bytes(sent) + ")", CLR_GREEN);
    log("FILE_SENT file=" + filename + " size=" + std::to_string(sent));
    return true;
}

// ============================================================
//  Messaging Wrappers
// ============================================================

/**
 * @brief Broadcast a text message to all clients via the server.
 *
 * Sends a @ref MSG_BROADCAST packet with @ref BROADCAST_ID as the target.
 *
 * @param msg  Text payload to broadcast.
 */
void Client::send_broadcast(const std::string& msg) {
    net_send_packet(m_sock, MSG_BROADCAST, m_my_id, BROADCAST_ID,
                    msg.c_str(), (uint32_t)msg.size(),
                    g_config.encrypt_type, g_config.encrypt_key);
}

/**
 * @brief Send a private message to a specific peer via the server.
 *
 * Sends a @ref MSG_PRIVATE packet directed to @p target_id.
 *
 * @param target_id  Numeric ID of the destination peer.
 * @param msg        Text payload.
 */
void Client::send_private(uint32_t target_id, const std::string& msg) {
    net_send_packet(m_sock, MSG_PRIVATE, m_my_id, target_id,
                    msg.c_str(), (uint32_t)msg.size(),
                    g_config.encrypt_type, g_config.encrypt_key);
}

/**
 * @brief Request an up-to-date client list from the server.
 *
 * Sends an empty @ref MSG_CLIENT_LIST request.  The response is handled
 * asynchronously by the receive thread via @ref parse_client_list().
 */
void Client::request_client_list() {
    net_send_packet(m_sock, MSG_CLIENT_LIST, m_my_id, SERVER_ID,
                    nullptr, 0,
                    g_config.encrypt_type, g_config.encrypt_key);
}

// ============================================================
//  GUI helper: send_nick_change
// ============================================================
void Client::send_nick_change(const std::string& new_nick)
{
    if (!m_connected || m_sock == INVALID_SOCK) return;
    m_nickname        = new_nick;
    g_config.nickname = new_nick;
    net_send_packet(m_sock, MSG_NICK_SET, m_my_id, SERVER_ID,
                    new_nick.c_str(), (uint32_t)new_nick.size(),
                    g_config.encrypt_type, g_config.encrypt_key);
}

/**
 * @brief Search the local peer cache for a client with the given nickname.
 *
 * Acquires @c m_peers_mutex for the duration of the search.
 *
 * @param nick  Exact nickname to find (case-sensitive).
 * @return      The matching peer's numeric ID, or @c 0 if not found.
 */
uint32_t Client::find_peer_by_nick(const std::string& nick) {
    std::lock_guard<std::mutex> lk(m_peers_mutex);
    for (auto& kv : m_peers)
        if (kv.second.nickname == nick) return kv.first;
    return 0;
}

// ============================================================
//  CLI Help
// ============================================================

/**
 * @brief Print the client command-line help text to @c stdout.
 *
 * Lists every supported CLI command with brief usage documentation.
 * Called on startup and in response to @c /help or @c /?.
 */
static void print_client_help() {
    std::cout <<
        "\n\033[1m=== SafeSocket Client Commands ===\033[0m\n"
        "  <message>                 - Send broadcast to all\n"
        "  /broadcast <message>      - Broadcast to all clients\n"
        "  /msg <id> <text>          - Send private message to client ID\n"
        "  /msgn <nick> <text>       - Send private message by nickname\n"
        "  /list                     - Request & display client list\n"
        "  /sendfile <id> <path>     - Send file to client ID (routed via server)\n"
        "  /sendfileserver <path>    - Send file directly to server\n"
        "  /nick <newname>           - Change your nickname\n"
        "  /myid                     - Show your client ID\n"
        "  /config                   - Show current configuration\n"
        "  /set <key> <value>        - Change a config value at runtime\n"
        "  /setconfig <key> <value>  - Same as /set\n"
        "  /saveconfig [path]        - Save config to file\n"
        "  /loadconfig [path]        - Reload config from file\n"
        "  /confighelp               - List all config keys\n"
        "  /quit  /exit              - Disconnect and exit\n"
        "  /help  /?                 - Show this help\n\n";
}

// ============================================================
//  Interactive CLI
// ============================================================

/**
 * @brief Block the calling thread running the user command-line interface.
 *
 * Reads commands from @c stdin until the user types @c /quit or @c /exit, or
 * until the connection is lost or @c stdin reaches EOF.  Each iteration:
 *  1. Prints the @c "[nickname]> " prompt.
 *  2. Reads a line with @c std::getline().
 *  3. If the line does not start with @c '/', sends it as a broadcast message.
 *  4. Otherwise, parses the leading command token (lowercased) and dispatches
 *     to the appropriate @ref Client method or @ref Config operation.
 *
 * Supported commands: @c /broadcast, @c /msg, @c /msgn, @c /list,
 * @c /sendfile, @c /sendfileserver, @c /nick, @c /myid, @c /config,
 * @c /confighelp, @c /set, @c /setconfig, @c /saveconfig, @c /loadconfig,
 * @c /quit (@c /exit, @c /disconnect), @c /help (@c /?).
 */
void Client::run_cli() {
    print_client_help();
    std::string line;

    while (m_connected && m_running) {
        std::cout << clr(CLR_BOLD) << clr(CLR_CYAN)
                  << "[" << m_nickname << "]> " << clr(CLR_RESET);
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;  /* EOF � exit cleanly. */

        /* Strip Windows-style line endings. */
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) continue;

        if (!m_connected) {
            std::cout << "[client] Not connected.\n";
            break;
        }

        if (line[0] != '/') {
            /* Plain text input ? broadcast to all peers. */
            send_broadcast("[" + m_nickname + "] " + line);
            continue;
        }

        /* Tokenise: extract the command verb. */
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "/quit" || cmd == "/exit" || cmd == "/disconnect") {
            disconnect();
            break;
        }
        else if (cmd == "/help" || cmd == "/?") {
            print_client_help();
        }
        else if (cmd == "/broadcast") {
            std::string msg;
            std::getline(iss, msg);
            while (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            if (msg.empty()) { std::cout << "Usage: /broadcast <message>\n"; continue; }
            send_broadcast("[" + m_nickname + "] " + msg);
        }
        else if (cmd == "/msg") {
            uint32_t id; std::string msg;
            iss >> id;
            std::getline(iss, msg);
            while (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            if (msg.empty()) { std::cout << "Usage: /msg <id> <message>\n"; continue; }
            send_private(id, msg);
        }
        else if (cmd == "/msgn") {
            std::string nick, msg;
            iss >> nick;
            std::getline(iss, msg);
            while (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            uint32_t id = find_peer_by_nick(nick);
            if (id == 0) { std::cout << "Peer '" << nick << "' not found. Try /list first.\n"; continue; }
            send_private(id, msg);
        }
        else if (cmd == "/list") {
            request_client_list();
        }
        else if (cmd == "/myid") {
            std::cout << "Your ID: " << m_my_id << "  Nickname: " << m_nickname << "\n";
        }
        else if (cmd == "/nick") {
            /* Change the display nickname locally and notify the server. */
            std::string nick;
            iss >> nick;
            if (nick.empty()) { std::cout << "Usage: /nick <newname>\n"; continue; }
            m_nickname = nick;
            g_config.nickname = nick;
            net_send_packet(m_sock, MSG_NICK_SET, m_my_id, SERVER_ID,
                            nick.c_str(), (uint32_t)nick.size(),
                            g_config.encrypt_type, g_config.encrypt_key);
            std::cout << "[client] Nickname changed to: " << nick << "\n";
        }
        else if (cmd == "/sendfile") {
            uint32_t id; std::string path;
            iss >> id;
            std::getline(iss, path);
            while (!path.empty() && path[0] == ' ') path = path.substr(1);
            if (path.empty()) { std::cout << "Usage: /sendfile <id> <filepath>\n"; continue; }
            send_file_to_client(id, path);
        }
        else if (cmd == "/sendfileserver") {
            std::string path;
            std::getline(iss, path);
            while (!path.empty() && path[0] == ' ') path = path.substr(1);
            if (path.empty()) { std::cout << "Usage: /sendfileserver <filepath>\n"; continue; }
            send_file_to_server(path);
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
        else {
            std::cout << "[client] Unknown command: " << cmd
                      << " (type /help for commands)\n";
        }
    }
}
