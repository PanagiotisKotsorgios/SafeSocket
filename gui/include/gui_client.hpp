#pragma once

/**
 * @file gui_client.hpp
 * @brief Thin GUI adapter wrapping the SafeSocket Client.
 *
 * GuiClient sits between the ImGui main loop (main thread) and the
 * background receive thread spawned by Client.  All callbacks from the
 * receive thread enqueue events into thread-safe queues that the main
 * thread drains each frame via poll().
 *
 * Usage:
 * @code
 * GuiClient gc;
 * if (gc.connect("127.0.0.1", 9000)) {
 *     // each ImGui frame:
 *     while (auto ev = gc.poll()) { ... }
 * }
 * @endcode
 */

#include "../../src/client.hpp"

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <functional>

// ── GuiClientEvent ────────────────────────────────────────────────────────────

/**
 * @brief One event produced by the background receive thread for the GUI.
 */
struct GuiClientEvent {
    enum class Type {
        Message,       ///< Normal broadcast or private text message
        SystemMsg,     ///< Server notification (join, kick, etc.)
        PeerListUpdate,///< MSG_CLIENT_LIST received — refresh peer cache
        Disconnected,  ///< Connection dropped
        FileRequest,   ///< Incoming file transfer request
        FileProgress,  ///< Bytes transferred update (v0.0.4+)
        FileComplete,  ///< Transfer finished
        FileRejected,  ///< Remote peer rejected the transfer
    };

    Type        type;
    std::string sender;     ///< Sender nickname for Message events
    std::string text;       ///< Message body or filename
    bool        is_private = false;
    uint64_t    bytes      = 0;  ///< For FileProgress
    uint32_t    peer_id    = 0;
};

// ── GuiClient ─────────────────────────────────────────────────────────────────

/**
 * @brief GUI adapter around @ref Client.
 *
 * Thread safety: connect(), disconnect(), send_*() are called from the
 * main thread.  The Client receive thread enqueues GuiClientEvents via
 * push_event() which is mutex-protected.  poll() drains the queue on
 * the main thread.
 */
class GuiClient {
public:
    GuiClient();
    ~GuiClient();

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Establish a connection to a SafeSocket server.
     * @return true on success, false if the connection was refused.
     */
    bool connect(const std::string& host, int port);

    /** Disconnect and stop the receive thread. */
    void disconnect();

    /** @return true if a connection is currently active. */
    bool is_connected() const;

    // ── Sending ───────────────────────────────────────────────────────────────

    void send_broadcast(const std::string& text);
    void send_private(uint32_t peer_id, const std::string& text);
    void send_file(uint32_t peer_id, const std::string& path);
    void send_nick_change(const std::string& new_nick);

    // ── Event queue ───────────────────────────────────────────────────────────

    /**
     * @brief Drain one event from the queue.
     * @return true and fills @p ev if an event was available; false if empty.
     */
    bool poll(GuiClientEvent& ev);

    /** @return A snapshot of the current peer list. */
    std::vector<PeerInfo> peers() const;

    /** @return This client's own numeric ID (0 until first CLIENT_LIST). */
    uint32_t my_id() const;

private:
    Client               m_client;
    mutable std::mutex   m_queue_mutex;
    std::queue<GuiClientEvent> m_queue;

    /** Called by the receive thread to enqueue an event. */
    void push_event(GuiClientEvent ev);

    // Callbacks wired up after connect() succeeds
    void setup_callbacks();
};
