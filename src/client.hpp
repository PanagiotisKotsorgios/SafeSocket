/**
 * @file client.hpp
 * @brief TCP client for SafeSocket with messaging, file transfer, and CLI.
 *
 * Declares the @ref PeerInfo helper structure and the @ref Client class.  The
 * client maintains a single persistent TCP connection to a SafeSocket server,
 * receives incoming packets on a background thread, and exposes an interactive
 * command-line interface that the caller drives by invoking @ref Client::run_cli().
 *
 * Typical lifecycle:
 * @code
 * Client client;
 * if (client.connect("127.0.0.1", 9000)) {
 *     client.run_cli();   // blocks until the user types /quit
 * }
 * @endcode
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#pragma once

#include "network.hpp"
#include "protocol.hpp"
#include "config.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

// ============================================================
//  PeerInfo
// ============================================================

/**
 * @brief Lightweight snapshot of a peer client's public identity.
 *
 * Populated from @ref MSG_CLIENT_LIST packets and stored in the local peer
 * cache so that the user can address other clients by nickname rather than
 * by raw numeric ID.
 */
struct PeerInfo {
    uint32_t    id;        ///< Numeric ID assigned to the peer by the server.
    std::string nickname;  ///< Display name reported by the peer in its @ref MSG_CONNECT handshake.
};

// ============================================================
//  Client
// ============================================================

/**
 * @brief SafeSocket TCP client with real-time messaging and file transfer.
 *
 * A @ref Client instance represents one connection to a SafeSocket server.
 * It spawns a background receive thread (@ref recv_loop) that runs concurrently
 * with the main CLI thread (@ref run_cli).  All shared state between the two
 * threads is protected by @c m_peers_mutex.
 *
 * ### Thread Safety
 * @ref m_peers and @ref m_my_id are written only by the receive thread.
 * @c m_connected and @c m_running are @c std::atomic<bool> and safe to read
 * from any thread without a lock.
 */
class Client {
public:
    // --------------------------------------------------------
    //  Constructor / Destructor
    // --------------------------------------------------------

    /**
     * @brief Constructs an idle, unconnected client.
     *
     * All handles are set to invalid/false; call @ref connect() to establish
     * a connection before using any other method.
     */
    Client();

    /**
     * @brief Destructor; calls @ref disconnect() to release all resources.
     */
    ~Client();

    // --------------------------------------------------------
    //  Connection Management
    // --------------------------------------------------------

    /**
     * @brief Establish a TCP connection to a SafeSocket server.
     *
     * Creates the download directory, resolves the nickname from
     * @c g_config.nickname, calls @ref net_connect(), starts the receive thread,
     * and sends the @ref MSG_CONNECT handshake (including the optional access key).
     *
     * @param host  Dotted-decimal IPv4 address of the server.
     * @param port  TCP port the server is listening on.
     * @return      @c true if the connection was established and the handshake sent,
     *              @c false on failure.
     */
    bool connect(const std::string& host, int port);

    /**
     * @brief Gracefully disconnect from the server and clean up resources.
     *
     * Sends @ref MSG_DISCONNECT, closes the socket, and joins the receive thread.
     * Safe to call multiple times; subsequent calls after the first are no-ops.
     */
    void disconnect();

    /**
     * @brief Run the interactive command-line interface until the user quits.
     *
     * Blocks the calling thread reading commands from @c stdin.  Supported
     * commands include @c /broadcast, @c /msg, @c /msgn, @c /list,
     * @c /sendfile, @c /sendfileserver, @c /nick, @c /myid, @c /config,
     * @c /set, and @c /quit.
     *
     * Plain text entered without a leading @c / is sent as a broadcast message.
     * Returns when the user disconnects or @c stdin reaches EOF.
     */
    void run_cli();

    // --------------------------------------------------------
    //  Messaging — Public API
    // --------------------------------------------------------

    /**
     * @brief Broadcast a text message to all clients via the server.
     *
     * Sends a @ref MSG_BROADCAST packet with @ref BROADCAST_ID as the target.
     *
     * @param msg  Text payload to broadcast.
     */
    void send_broadcast(const std::string& msg);

    /**
     * @brief Send a private text message to a specific peer via the server.
     *
     * Sends a @ref MSG_PRIVATE packet directed to @p target_id.
     *
     * @param target_id  Numeric ID of the destination peer.
     * @param msg        Text payload.
     */
    void send_private(uint32_t target_id, const std::string& msg);

    /**
     * @brief Ask the server to send an up-to-date list of connected clients.
     *
     * Sends a @ref MSG_CLIENT_LIST request.  The response is handled asynchronously
     * by the receive thread and updates @c m_peers.
     */
    void request_client_list();

    // --------------------------------------------------------
    //  File Transfer — Public API
    // --------------------------------------------------------

    /**
     * @brief Send a local file directly to the server for storage.
     *
     * Initiates the MSG_FILE_START ? MSG_FILE_DATA… ? MSG_FILE_END exchange
     * with the server as the target.
     *
     * @param filepath  Local filesystem path of the file to upload.
     * @return          @c true if the transfer completed successfully, @c false otherwise.
     */
    bool send_file_to_server(const std::string& filepath);

    /**
     * @brief Send a local file to another client, routed through the server.
     *
     * Identical to @ref send_file_to_server() but sets the @c target_id in the
     * file-start packet to @p target_id so the server can forward it to the
     * appropriate peer.
     *
     * @param target_id  Numeric ID of the destination peer.
     * @param filepath   Local filesystem path of the file to send.
     * @return           @c true if the transfer completed successfully, @c false otherwise.
     */
    bool send_file_to_client(uint32_t target_id, const std::string& filepath);

    // --------------------------------------------------------
    //  Accessors
    // --------------------------------------------------------

    /**
     * @brief Return this client's numeric ID as assigned by the server.
     *
     * The ID is populated when the server sends the @c "YOUR_ID=" @ref MSG_SERVER_INFO
     * packet during the connection handshake.  Returns @c 0 before the ID has been received.
     *
     * @return The server-assigned client ID.
     */
    uint32_t my_id() const { return m_my_id; }

    /**
     * @brief Return @c true if the client has an active connection to the server.
     *
     * Thread-safe; the flag is an @c std::atomic<bool>.
     *
     * @return @c true when connected and the receive thread is running.
     */
    bool     is_connected() const { return m_connected; }

private:
    // --------------------------------------------------------
    //  Internal State
    // --------------------------------------------------------

    sock_t              m_sock;          ///< Connected socket handle; @ref INVALID_SOCK when not connected.
    std::atomic<bool>   m_connected;     ///< @c true while the socket connection is alive.
    std::atomic<bool>   m_running;       ///< @c true while the receive thread should continue executing.
    std::thread         m_recv_thread;   ///< Background thread executing @ref recv_loop().

    uint32_t            m_my_id;         ///< Server-assigned numeric ID; populated on handshake completion.
    std::string         m_nickname;      ///< Local copy of the current display name (mirrors @c g_config.nickname after any @c /nick change).

    std::map<uint32_t, PeerInfo> m_peers; ///< Cache of peer identities updated from @ref MSG_CLIENT_LIST responses.
    std::mutex          m_peers_mutex;    ///< Protects all accesses to @c m_peers.

    /// @name Incoming-File State
    /// These fields track the state of an in-progress incoming file transfer.
    ///@{
    std::atomic<bool>   m_file_incoming;       ///< Set to @c true while a file transfer is being received.
    std::string         m_file_incoming_name;  ///< Sanitized filename of the file currently being received.
    uint64_t            m_file_incoming_size;  ///< Total expected byte count of the incoming file.
    ///@}

    // --------------------------------------------------------
    //  Internal Thread and Packet Handling
    // --------------------------------------------------------

    /**
     * @brief Main loop of the background receive thread.
     *
     * Continuously calls @ref net_recv_packet() and dispatches each received
     * packet to @ref handle_packet().  Terminates when @c m_running is @c false
     * or the socket is closed.
     */
    void recv_loop();

    /**
     * @brief Dispatch a received packet to the appropriate handler.
     *
     * Handles MSG_SERVER_INFO, MSG_BROADCAST, MSG_PRIVATE, MSG_TEXT,
     * MSG_CLIENT_LIST, MSG_ACK, MSG_ERROR, MSG_KICK, MSG_PING, and MSG_FILE_START.
     * Unknown types are reported when @c g_config.verbose is @c true.
     *
     * @param hdr      Parsed @ref PacketHeader of the received packet.
     * @param payload  Decrypted payload bytes of the received packet.
     */
    void handle_packet(const PacketHeader& hdr,
                       const std::vector<uint8_t>& payload);

    /**
     * @brief Receive and save a complete incoming file transfer.
     *
     * Called from @ref handle_packet() when @ref MSG_FILE_START is received.
     * Validates the file size against @c g_config.max_file_size, sends
     * @ref MSG_FILE_ACCEPT or @ref MSG_FILE_REJECT, then reads @ref MSG_FILE_DATA
     * chunks and writes them to disk until @ref MSG_FILE_END is received.
     *
     * The filename is sanitized to prevent directory-traversal attacks before
     * the output path is constructed under @c g_config.download_dir.
     *
     * @param hdr           Header of the MSG_FILE_START packet (used for sender_id).
     * @param start_payload Payload bytes of the MSG_FILE_START packet containing
     *                      the @ref FileStartPayload struct followed by the filename.
     */
    void handle_incoming_file(const PacketHeader& hdr,
                              const std::vector<uint8_t>& start_payload);

    /**
     * @brief Parse a @ref MSG_CLIENT_LIST payload and refresh @c m_peers.
     *
     * Decodes the [count][id][nick_len][nick…] binary format, rebuilds the
     * @c m_peers map under @c m_peers_mutex, and prints the peer list to @c stdout.
     *
     * @param payload  Raw payload bytes of the @ref MSG_CLIENT_LIST packet.
     */
    void parse_client_list(const std::vector<uint8_t>& payload);

    // --------------------------------------------------------
    //  File Transfer Core
    // --------------------------------------------------------

    /**
     * @brief Execute the client-side file send protocol over a given socket.
     *
     * Internal implementation shared by @ref send_file_to_server() and
     * @ref send_file_to_client().  Performs:
     *  -# Open the file and measure its size.
     *  -# Send @ref MSG_FILE_START with the @ref FileStartPayload and filename.
     *  -# Wait for @ref MSG_FILE_ACCEPT or @ref MSG_FILE_REJECT.
     *  -# Stream @ref MSG_FILE_DATA chunks of up to @ref FILE_CHUNK bytes.
     *  -# Send @ref MSG_FILE_END.
     *
     * @param sock       Socket to use for the transfer.
     * @param target_id  Target ID embedded in the packet headers (server or peer).
     * @param filepath   Local filesystem path of the file to transmit.
     * @return           @c true on successful completion, @c false on any error or rejection.
     */
    bool do_send_file(sock_t sock, uint32_t target_id, const std::string& filepath);

    // --------------------------------------------------------
    //  Output Helpers
    // --------------------------------------------------------

    /**
     * @brief Print a coloured message to @c stdout and log it.
     *
     * Prefixes output with @c "\r" to overwrite the current prompt line, then
     * reprints the prompt after the message so the input line remains at the
     * bottom of the terminal.
     *
     * @param msg    The message string to display.
     * @param color  ANSI colour escape sequence to wrap the message in, or
     *               @c nullptr for plain output.
     */
    void print(const std::string& msg, const char* color = nullptr);

    /**
     * @brief Append a timestamped log entry to the log file.
     *
     * Opens @c g_config.log_file in append mode, writes the entry, and closes the
     * file.  Does nothing if @c g_config.log_to_file is @c false.
     *
     * @param msg  The log message string to write (without trailing newline).
     */
    void log(const std::string& msg);

    /**
     * @brief Search the local peer cache for a client with the given nickname.
     *
     * Acquires @c m_peers_mutex for the duration of the search.
     *
     * @param nick  Exact nickname string to find (case-sensitive).
     * @return      The matching peer's numeric ID, or @c 0 if not found.
     */
    uint32_t find_peer_by_nick(const std::string& nick);
};
