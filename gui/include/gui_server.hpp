#pragma once

/**
 * @file gui_server.hpp
 * @brief Thin GUI adapter wrapping the SafeSocket Server.
 *
 * GuiServer starts and stops the server and exposes a thread-safe event
 * queue so the main ImGui thread can react to client connections,
 * disconnections, messages, and file transfers without touching the
 * Server internals directly.
 */

#include "../../src/server.hpp"

#include <string>
#include <vector>
#include <queue>
#include <mutex>

// ── GuiServerEvent ────────────────────────────────────────────────────────────

/**
 * @brief One event produced by the Server for the GUI.
 */
struct GuiServerEvent {
    enum class Type {
        ClientConnected,    ///< A new client joined
        ClientDisconnected, ///< A client left or was kicked
        Message,            ///< Broadcast or private message received
        FileTransferStart,  ///< A file transfer was initiated
        FileTransferEnd,    ///< A file transfer completed or was rejected
        AuthFailure,        ///< A connect attempt with a bad access key
        Error,              ///< Internal server error
    };

    Type        type;
    uint32_t    client_id  = 0;
    std::string nickname;
    std::string ip;
    std::string text;       ///< Message body, filename, or error description
    bool        is_private = false;
};

// ── GuiServerClientInfo ───────────────────────────────────────────────────────

/**
 * @brief Lightweight snapshot of one connected client for the GUI table.
 */
struct GuiServerClientInfo {
    uint32_t    id;
    std::string nickname;
    std::string ip;
    std::time_t connected_at;
};

// ── GuiServer ─────────────────────────────────────────────────────────────────

/**
 * @brief GUI adapter around @ref Server.
 *
 * start() launches the server's accept loop in a background thread.
 * stop() shuts it down cleanly.  All GUI interaction goes through the
 * event queue drained by poll().
 */
class GuiServer {
public:
    GuiServer();
    ~GuiServer();

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the server listening on host:port.
     * @return true if the bind and listen succeeded.
     */
    bool start(const std::string& host, int port);

    /** Stop the server and disconnect all clients. */
    void stop();

    /** @return true if the server is currently running. */
    bool is_running() const;

    // ── Commands ──────────────────────────────────────────────────────────────

    void broadcast(const std::string& text);
    void private_msg(uint32_t client_id, const std::string& text);
    void kick(uint32_t client_id, const std::string& reason = "");
    void send_file_to_client(uint32_t client_id, const std::string& path);
    void send_file_to_all(const std::string& path);

    // ── Event queue ───────────────────────────────────────────────────────────

    /**
     * @brief Drain one event from the queue.
     * @return true and fills @p ev if an event was available.
     */
    bool poll(GuiServerEvent& ev);

    /** @return Snapshot of connected clients for display. */
    std::vector<GuiServerClientInfo> client_list() const;

    /** @return Number of currently connected clients. */
    size_t client_count() const;

private:
    Server             m_server;
    mutable std::mutex m_queue_mutex;
    std::queue<GuiServerEvent> m_queue;

    void push_event(GuiServerEvent ev);
    void setup_callbacks();
};
