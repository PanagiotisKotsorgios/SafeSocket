#ifndef APP_HPP
#include "security.hpp"
#define APP_HPP

#include <string>
#include <deque>
#include <vector>

enum class AppScreen {
    Connect,
    Client,
    Server
};

struct ChatMessage {
    std::string sender;
    std::string text;
    bool is_private;
    bool is_system;
    ChatMessage(const std::string& s, const std::string& t,
                bool priv = false, bool sys = false)
        : sender(s), text(t), is_private(priv), is_system(sys) {}
};

struct FileTransferEntry {
    std::string filename;
    std::string peer;
    float       progress;
    bool        is_upload;
    bool        done;
    bool        failed;
    FileTransferEntry()
        : progress(0.0f), is_upload(false), done(false), failed(false) {}
};

struct AppState {
    AppScreen screen;

    std::deque<ChatMessage>        messages;
    static const size_t            MAX_MESSAGES = 2000;
    std::vector<FileTransferEntry> transfers;

    std::string status_msg;
    bool        status_is_error;

    // ── Connect screen inputs ────────────────────────────────────────────
    char cs_host[128];
    int  cs_port;
    char cs_nick[64];
    int  cs_encrypt;
    char cs_key[256];
    char cs_access_key[256];
    bool cs_require_key;

    // ── Extra connect options ────────────────────────────────────────────
    int  cs_timeout_sec;        // connection timeout in seconds
    bool cs_auto_reconnect;     // auto reconnect on drop
    char cs_profile_name[64];   // profile name for save/load

    // ── Connection log ───────────────────────────────────────────────────
    std::deque<std::string> conn_log;
    static const size_t     MAX_LOG = 200;

    // ── Theme ────────────────────────────────────────────────────────────
    bool dark_theme;

    // ── Modal flags ──────────────────────────────────────────────────────
    bool show_about;
    bool show_help;

    SecurityState sec;

    AppState();
    void push_message(const std::string& sender, const std::string& text,
                      bool is_private = false, bool is_system = false);
    void set_status(const std::string& msg, bool error = false);
    void log(const std::string& msg);
};

void draw_connect_window(AppState& app);
void draw_chat_window(AppState& app);
void draw_server_window(AppState& app);
void draw_files_window(AppState& app);
void draw_settings_window(AppState& app);
void draw_navbar(AppState& app);
void apply_dark_theme();
void apply_light_theme();

#endif
