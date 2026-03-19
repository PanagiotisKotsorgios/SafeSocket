/**
 * @file server.hpp
 * @brief Multi-client TCP server for SafeSocket.
 *
 * Declares the @ref ClientInfo structure and the @ref Server class.  The server
 * supports concurrent clients via per-client threads, real-time text messaging
 * (broadcast and private), file transfer in both directions, keepalive pings,
 * access-key authentication, nickname management, and an interactive CLI.
 *
 * Typical lifecycle:
 * @code
 * Server server;
 * if (server.start("0.0.0.0", 9000)) {
 *     server.run_cli();   // blocks until the operator types /quit
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
#include <fstream>
#include <functional>
#include <ctime>

// ============================================================
//  ClientInfo
// ============================================================

/**
 * @brief Holds all state associated with a single connected client.
 *
 * Instances are heap-allocated by the accept loop and stored in the server's
 * client map.  Each instance owns the client's socket and receive thread for
 * its lifetime.
 *
 * @note The @c thread member is @c detach()ed after creation to avoid blocking
 *       the accept loop.  Clean-up on disconnect relies on the @c alive flag.
 */
struct ClientInfo {
    uint32_t    id;            ///< Unique numeric identifier assigned by the server on connection.
    sock_t      sock;          ///< Connected socket handle used for all I/O with this client.
    std::string ip;            ///< Dotted-decimal IPv4 address of the remote endpoint.
    std::string nickname;      ///< Display name supplied by the client; defaults to @c "client_<id>".
    std::time_t connected_at;  ///< UNIX timestamp of the moment the connection was accepted.
    bool        authenticated; ///< @c true once the client has supplied a valid access key (or none is required).

    /**
     * @brief Atomic liveness flag polled by the receive loop.
     *
     * Set to @c false to request graceful shutdown of the client's thread.
     */
    std::atomic<bool> alive;

    /**
     * @brief Thread running @ref Server::client_loop() for this client.
     *
     * Detached immediately after construction so the accept loop can continue
     * without waiting for it.
     */
    std::thread thread;

    /**
     * @brief Constructs a @ref ClientInfo with safe default values.
     *
     * The @c sock is initialised to @ref INVALID_SOCK and @c alive to @c false;
     * both must be set explicitly by the accept loop before use.
     */
    ClientInfo() : id(0), sock(INVALID_SOCK), connected_at(0),
                   authenticated(false), alive(false) {}
};

// ============================================================
//  Server
// ============================================================

/**
 * @brief TCP chat-and-file-transfer server managing multiple concurrent clients.
 *
 * The server spawns one thread per connected client to handle receive loops,
 * one accept thread to listen for incoming connections, and an optional keepalive
 * thread to send periodic pings.  All access to the client map is protected by
 * @c m_clients_mutex.
 *
 * ### Thread Safety
 * Public methods that touch the client map acquire @c m_clients_mutex internally.
 * Do not call public methods from within a context that already holds this lock.
 */
class Server {
public:
    // --------------------------------------------------------
    //  Constructor / Destructor
    // --------------------------------------------------------

    /**
     * @brief Constructs an idle server with no open sockets or threads.
     *
     * Call @ref start() to begin accepting connections.
     */
    Server();

    /**
     * @brief Destructor; calls @ref stop() to release all resources.
     */
    ~Server();

    // --------------------------------------------------------
    //  Lifecycle
    // --------------------------------------------------------

    /**
     * @brief Bind to the given address and start accepting client connections.
     *
     * Creates the download directory, opens the log file if configured, creates
     * the listening socket via @ref net_listen(), and launches the accept and
     * (optionally) keepalive threads.
     *
     * @param host  IPv4 address to bind to; @c "0.0.0.0" binds to all interfaces.
     * @param port  TCP port to listen on.
     * @return      @c true on successful bind and listen, @c false on failure.
     */
    bool start(const std::string& host, int port);

    /**
     * @brief Gracefully shut down the server.
     *
     * Closes the listening socket, signals all client threads to stop, joins the
     * accept and keepalive threads, and releases all client resources.  Safe to
     * call multiple times; subsequent calls are no-ops.
     */
    void stop();

    /**
     * @brief Run the interactive command-line interface until the operator quits.
     *
     * Blocks the calling thread reading from @c stdin.  Supports commands such as
     * @c /list, @c /broadcast, @c /msg, @c /kick, @c /sendfile, @c /stats,
     * @c /config, and @c /quit.  Returns when @c /quit (or equivalent) is entered
     * or @c stdin reaches EOF.
     */
    void run_cli();

    // --------------------------------------------------------
    //  Messaging � Public API
    // --------------------------------------------------------

    /**
     * @brief Send a text message to all connected clients.
     *
     * Emits a @ref MSG_BROADCAST packet to every client in the session and logs
     * the message server-side.
     *
     * @param msg      Text to broadcast.
     * @param from_id  Sender ID placed in the packet header; defaults to @ref SERVER_ID.
     */
    void broadcast_text(const std::string& msg, uint32_t from_id = SERVER_ID);

    /**
     * @brief Send a private text message to a specific client.
     *
     * Emits a @ref MSG_PRIVATE packet to the client identified by @p target_id.
     * Logs an error if the target is not found.
     *
     * @param target_id  Numeric ID of the destination client.
     * @param msg        Text payload.
     * @param from_id    Sender ID placed in the packet header; defaults to @ref SERVER_ID.
     */
    void send_text(uint32_t target_id, const std::string& msg, uint32_t from_id = SERVER_ID);

    /**
     * @brief Send a packet of an arbitrary type to a specific client.
     *
     * Useful for sending non-standard or administrative messages without going
     * through the higher-level wrappers.
     *
     * @param target_id  Numeric ID of the destination client.
     * @param type       @ref MsgType value identifying the packet purpose.
     * @param text       Text payload to include in the packet.
     */
    void send_raw_msg(uint32_t target_id, uint32_t type, const std::string& text);

    // --------------------------------------------------------
    //  File Transfer � Public API
    // --------------------------------------------------------

    /**
     * @brief Send a file from the server's filesystem to a single client.
     *
     * Performs the full MSG_FILE_START ? MSG_FILE_DATA� ? MSG_FILE_END exchange,
     * waiting for the client to accept or reject the transfer before sending data.
     *
     * @param target_id  Numeric ID of the destination client.
     * @param filepath   Absolute or relative path to the file on the server.
     * @return           @c true if the transfer completed without error, @c false otherwise.
     */
    bool send_file(uint32_t target_id, const std::string& filepath);

    /**
     * @brief Send a file from the server's filesystem to every connected client.
     *
     * Calls @ref send_file() for each client sequentially.
     *
     * @param filepath  Absolute or relative path to the file on the server.
     * @return          @c true if all individual transfers succeeded, @c false if any failed.
     */
    bool send_file_all(const std::string& filepath);

    // --------------------------------------------------------
    //  Utilities � Public API
    // --------------------------------------------------------

    /**
     * @brief Print a formatted table of connected clients to @c stdout.
     *
     * Columns: ID, nickname, IP address, and connection time.
     */
    void list_clients();

    /**
     * @brief Forcefully disconnect a client.
     *
     * Sends a @ref MSG_KICK packet with an optional reason, then closes the
     * client's socket and marks it as no longer alive.
     *
     * @param id      Numeric ID of the client to kick.
     * @param reason  Optional human-readable reason string appended to the kick message.
     */
    void kick_client(uint32_t id, const std::string& reason = "");

    // GUI helpers -----------------------------------------------
    struct ClientSnapshot {
        uint32_t    id;
        std::string nickname;
        std::string ip;
        bool        authenticated;
        std::time_t connected_at;
    };
    std::vector<ClientSnapshot> get_clients_snapshot();
    void broadcast_server_message(const std::string& msg);

    // GUI message callback — routes Server::print() into the GUI
    using ServerMsgCallback = std::function<void(const std::string& msg, bool is_error)>;
    void set_message_callback(ServerMsgCallback cb) { m_srv_msg_cb = cb; }

    // Stats snapshot for GUI
    struct StatsSnapshot {
        size_t   clients_online;
        uint64_t msg_count;
        uint64_t bytes_sent;
        uint64_t bytes_recv;
        std::string encrypt_type;
    };
    StatsSnapshot get_stats();
    // -----------------------------------------------------------

    /**
     * @brief Print server statistics to @c stdout.
     *
     * Includes the number of active clients, total messages routed, bytes sent,
     * bytes received, and the active encryption type.
     */
    void print_stats();

    /**
     * @brief Look up a client's numeric ID by their nickname.
     *
     * @param nick  The exact nickname string to search for (case-sensitive).
     * @return      The client's numeric ID if found, or @c 0 if no match exists.
     */
    uint32_t find_client_by_nick(const std::string& nick);

private:
    // --------------------------------------------------------
    //  Internal State
    // --------------------------------------------------------

    sock_t              m_listen_sock;    ///< Listening socket created by @ref net_listen(); @ref INVALID_SOCK when not running.
    std::atomic<bool>   m_running;        ///< Global run flag; set to @c false by @ref stop() to terminate all loops.
    std::thread         m_accept_thread;  ///< Thread executing @ref accept_loop().
    std::thread         m_keepalive_thread; ///< Thread executing @ref keepalive_loop(); only started when keepalive is enabled.

    std::map<uint32_t, ClientInfo*> m_clients; ///< Map from client ID to heap-allocated @ref ClientInfo.
    std::mutex          m_clients_mutex;  ///< Protects all accesses to @c m_clients.
    ServerMsgCallback   m_srv_msg_cb;     ///< Optional GUI output callback.
    uint32_t            m_next_id;        ///< Monotonically increasing counter used to assign unique client IDs.
    std::ofstream       m_log_file;       ///< Output file stream for persistent logging; open only when @c g_config.log_to_file is true.
    uint64_t            m_bytes_sent;     ///< Running total of bytes transmitted to clients (payload only, excluding headers).
    uint64_t            m_bytes_recv;     ///< Running total of bytes received from clients (payload only, excluding headers).
    uint64_t            m_msg_count;      ///< Running total of messages routed or generated by the server.

    // --------------------------------------------------------
    //  Internal Thread Loops
    // --------------------------------------------------------

    /**
     * @brief Main loop of the accept thread.
     *
     * Calls @ref net_accept() in a tight loop.  For each successful accept it
     * allocates a @ref ClientInfo, inserts it into @c m_clients, and spawns a
     * detached thread running @ref client_loop().  Rejects connections when
     * @c m_clients.size() >= @c g_config.max_clients.
     */
    void accept_loop();

    /**
     * @brief Per-client receive loop executed on a dedicated thread.
     *
     * Sends the MOTD and the assigned client ID, then enters a packet receive
     * loop calling @ref handle_packet() for each message.  On exit it notifies
     * other clients of the disconnection and calls @ref remove_client().
     *
     * @param ci  Pointer to the @ref ClientInfo for the client being served.
     */
    void client_loop(ClientInfo* ci);

    /**
     * @brief Periodic keepalive thread loop.
     *
     * Sleeps for @c g_config.ping_interval seconds between iterations.  On each
     * wakeup it sends a @ref MSG_PING packet to every connected client.
     * Runs until @c m_running becomes @c false.
     */
    void keepalive_loop();

    // --------------------------------------------------------
    //  Packet Dispatch
    // --------------------------------------------------------

    /**
     * @brief Dispatch a received packet to the appropriate handler.
     *
     * Called from @ref client_loop() with a fully parsed header and decrypted
     * payload.  Handles MSG_CONNECT, MSG_TEXT, MSG_BROADCAST, MSG_PRIVATE,
     * MSG_CLIENT_LIST, MSG_NICK_SET, MSG_PING, MSG_FILE_START, and MSG_DISCONNECT.
     *
     * @param ci       The sending client's @ref ClientInfo.
     * @param hdr      The parsed @ref PacketHeader of the received packet.
     * @param payload  The decrypted payload bytes of the received packet.
     */
    void handle_packet(ClientInfo* ci, const PacketHeader& hdr,
                       const std::vector<uint8_t>& payload);

    /**
     * @brief Receive and store a complete file uploaded by a client.
     *
     * Called when a @ref MSG_FILE_START is detected by @ref handle_packet().
     * Reads the file metadata, sends @ref MSG_FILE_ACCEPT, then receives
     * @ref MSG_FILE_DATA chunks until @ref MSG_FILE_END is received.  Sanitizes
     * the filename to prevent path traversal before writing to disk.
     *
     * @param ci  The uploading client's @ref ClientInfo.
     */
    void handle_file_transfer(ClientInfo* ci);

    // --------------------------------------------------------
    //  Client Management Helpers
    // --------------------------------------------------------

    /**
     * @brief Remove a client from the internal map and close its socket.
     *
     * @param id  Numeric ID of the client to remove.
     *
     * @warning Acquires @c m_clients_mutex internally; do not call while the
     *          lock is already held by the current thread.
     */
    void remove_client(uint32_t id);

    /**
     * @brief Build and send the serialized client list to one or all clients.
     *
     * Encodes the current @c m_clients map as a @ref MSG_CLIENT_LIST payload and
     * dispatches it.
     *
     * @param target_id  Numeric ID of the intended recipient, or @c 0 to send to
     *                   every connected client.
     *
     * @warning Acquires @c m_clients_mutex internally; do not call while the
     *          lock is already held by the current thread.
     */
    void send_client_list(uint32_t target_id);

    /**
     * @brief Route a packet to one or all clients, optionally excluding one sender.
     *
     * If @c hdr.target_id equals @ref BROADCAST_ID the packet is sent to every
     * client except @p exclude_id.  Otherwise it is sent to the single matching
     * client.
     *
     * @param hdr        Header of the packet to route; @c msg_type and IDs are reused.
     * @param payload    Payload bytes to attach to each outgoing packet.
     * @param exclude_id Client ID to skip (used to avoid echoing back to the sender).
     */
    void distribute_packet(const PacketHeader& hdr,
                           const std::vector<uint8_t>& payload,
                           uint32_t exclude_id = 0xFFFFFFFEu);

    // --------------------------------------------------------
    //  File Transfer Helper
    // --------------------------------------------------------

    /**
     * @brief Execute the server-side file send protocol for one client.
     *
     * Internal implementation shared by @ref send_file() and @ref send_file_all().
     * Performs the full MSG_FILE_START ? wait-for-accept ? MSG_FILE_DATA� ?
     * MSG_FILE_END exchange with progress reporting when @c g_config.verbose is true.
     *
     * @param ci        Pointer to the target client's @ref ClientInfo.
     * @param filepath  Absolute or relative path to the file to transmit.
     * @return          @c true if the transfer completed, @c false on any error or rejection.
     */
    bool send_file_to_client(ClientInfo* ci, const std::string& filepath);

    // --------------------------------------------------------
    //  Logging Helpers
    // --------------------------------------------------------

    /**
     * @brief Append a plain-text log entry to the log file (if logging is enabled).
     *
     * Prefixes the message with a @c "YYYY-MM-DD HH:MM:SS" timestamp.
     * Does nothing if @c g_config.log_to_file is @c false or the file is not open.
     *
     * @param msg  The log message string to write.
     */
    void log(const std::string& msg);

    /**
     * @brief Print a message to @c stdout and simultaneously log it.
     *
     * Wraps the text in ANSI colour codes when @c g_config.color_output is true
     * and @p color is non-null.
     *
     * @param msg    The message string to print.
     * @param color  ANSI colour escape sequence to apply, or @c nullptr for no colour.
     */
    void print(const std::string& msg, const char* color = nullptr);
};
