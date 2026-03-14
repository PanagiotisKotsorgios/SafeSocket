#pragma once

/**
 * @file app.hpp
 * @brief Central application state shared across all GUI windows.
 *
 * AppState owns the GuiClient / GuiServer adapters and the message log.
 * Every draw_*_window() function receives a reference to the single AppState
 * instance that lives in main_gui.cpp.
 */

#include "gui_client.hpp"
#include "gui_server.hpp"
#include "../../src/config.hpp"

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <ctime>

// ── Screen / mode enum ────────────────────────────────────────────────────────

/**
 * @brief Which top-level screen is currently shown.
 */
enum class AppScreen {
    Connect,    ///< Initial connection/login screen
    Client,     ///< Connected as a client
    Server,     ///< Running as a server
};

// ── ChatMessage ───────────────────────────────────────────────────────────────

/**
 * @brief A single entry in the chat history.
 */
struct ChatMessage {
    std::time_t timestamp;   ///< UNIX time when received
    std::string sender;      ///< Sender nickname (or "Server")
    std::string text;        ///< Message body
    bool        is_private;  ///< True if this was a private message
    bool        is_system;   ///< True if this is a server/system notification
};

// ── FileTransferEntry ─────────────────────────────────────────────────────────

/**
 * @brief Tracks one active or completed file transfer for display.
 */
struct FileTransferEntry {
    std::string filename;
    std::string peer;        ///< Peer nickname
    uint64_t    total_bytes;
    uint64_t    bytes_done;
    bool        is_upload;   ///< true = sending, false = receiving
    bool        finished;
    bool        rejected;
};

// ── AppState ──────────────────────────────────────────────────────────────────

/**
 * @brief All mutable state shared across the GUI windows.
 *
 * Only the main thread reads/writes this struct directly.
 * The GuiClient and GuiServer adapters post events into thread-safe queues;
 * app.cpp drains those queues each frame before drawing.
 */
struct AppState {
    // ── Current screen ────────────────────────────────────────────────────────
    AppScreen screen = AppScreen::Connect;

    // ── Adapters ─────────────────────────────────────────────────────────────
    GuiClient  gui_client;
    GuiServer  gui_server;

    // ── Chat history ─────────────────────────────────────────────────────────
    std::deque<ChatMessage> messages;   ///< All received messages (newest at back)
    static constexpr size_t MAX_MESSAGES = 2000;

    // ── File transfers ────────────────────────────────────────────────────────
    std::vector<FileTransferEntry> transfers;

    // ── Connect-screen fields (bound to ImGui inputs) ─────────────────────────
    char   cs_host[128]     = "127.0.0.1";
    int    cs_port          = 9000;
    char   cs_nick[64]      = "anonymous";
    int    cs_encrypt       = 0;            ///< Index into ENCRYPT_NAMES[]
    char   cs_key[256]      = {};
    char   cs_access_key[256] = {};
    bool   cs_require_key   = false;

    // ── Chat-window input buffer ──────────────────────────────────────────────
    char   chat_input[1024] = {};

    // ── Settings dirty flag ───────────────────────────────────────────────────
    bool   settings_changed = false;

    // ── Error / status banner ─────────────────────────────────────────────────
    std::string status_msg;
    bool        status_is_error = false;

    // ── Helpers ───────────────────────────────────────────────────────────────

    /** Push a chat message, trimming to MAX_MESSAGES. */
    void push_message(const std::string& sender,
                      const std::string& text,
                      bool is_private = false,
                      bool is_system  = false) {
        ChatMessage m;
        m.timestamp  = std::time(nullptr);
        m.sender     = sender;
        m.text       = text;
        m.is_private = is_private;
        m.is_system  = is_system;
        messages.push_back(m);
        if (messages.size() > MAX_MESSAGES)
            messages.pop_front();
    }

    void set_status(const std::string& msg, bool error = false) {
        status_msg      = msg;
        status_is_error = error;
    }
};

// ── Forward declarations for draw functions ───────────────────────────────────
void draw_connect_window(AppState& app);
void draw_chat_window(AppState& app);
void draw_server_window(AppState& app);
void draw_files_window(AppState& app);
void draw_settings_window(AppState& app);

/** Called each frame to drain adapter queues and update AppState. */
void app_poll_events(AppState& app);
