#pragma once
#include "security.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "config.hpp"
#include "client.hpp"
#include "server.hpp"

#include <string>
#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <map>

// ── Screen ────────────────────────────────────────────────────
enum class AppScreen { Connect, Client, Server };

// ── ChatMessage ───────────────────────────────────────────────
struct ChatMessage {
    std::string sender;
    std::string text;
    bool        is_private;
    bool        is_system;
    std::string timestamp;
    uint32_t    from_id;
    ChatMessage(const std::string& s, const std::string& t,
                bool priv=false, bool sys=false, uint32_t fid=0);
};

// ── FileTransferEntry ─────────────────────────────────────────
struct FileTransferEntry {
    std::string filename;
    std::string peer;
    float       progress  = 0.0f;
    bool        is_upload = false;
    bool        done      = false;
    bool        failed    = false;
};

// ── GUIPeer (sidebar) ─────────────────────────────────────────
struct GUIPeer {
    uint32_t    id;
    std::string nickname;
};

// ── GUIClientRow (server panel) ───────────────────────────────
struct GUIClientRow {
    uint32_t    id;
    std::string nickname;
    std::string ip;
    std::string connected_at;
    bool        authenticated;
};

// ── AppState ──────────────────────────────────────────────────
struct AppState {
    // ── Screen ────────────────────────────────────────────────
    AppScreen screen;

    // ── Chat messages ─────────────────────────────────────────
    std::deque<ChatMessage> messages;
    std::mutex              messages_mutex;
    static const size_t     MAX_MESSAGES = 2000;

    // ── File transfers ────────────────────────────────────────
    std::vector<FileTransferEntry> transfers;
    std::mutex                     transfers_mutex;

    // ── Status bar ────────────────────────────────────────────
    std::string status_msg;
    bool        status_is_error;

    // ── Connection log ────────────────────────────────────────
    std::deque<std::string> conn_log;
    std::mutex              conn_log_mutex;
    static const size_t     MAX_LOG = 200;

    // ── Client connect form ───────────────────────────────────
    char cs_host[128];
    int  cs_port;
    char cs_nick[64];
    int  cs_encrypt;
    char cs_key[256];
    char cs_access_key[256];
    bool cs_require_key;
    int  cs_timeout_sec;
    bool cs_auto_reconnect;
    char cs_profile_name[64];

    // ── Server config form ────────────────────────────────────
    char sv_host[128];
    int  sv_port;
    char sv_name[128];
    char sv_motd[256];
    int  sv_max_clients;
    bool sv_require_key;
    char sv_access_key[256];
    int  sv_encrypt;
    char sv_encrypt_key[256];

    // ── Chat input ────────────────────────────────────────────
    char chat_input[1024];
    int  chat_pm_target;   // 0 = broadcast
    bool chat_pm_mode;

    // ── Settings ──────────────────────────────────────────────
    char set_download_dir[512];
    bool set_auto_accept;
    bool set_verbose;
    bool set_log_to_file;
    char set_log_file[512];
    int  set_max_file_mb;
    int  set_ping_interval;
    bool set_keepalive;

    // ── Appearance ────────────────────────────────────────────
    bool dark_theme;
    bool show_about;
    bool show_help;

    // ── Live client state ─────────────────────────────────────
    std::shared_ptr<Client> client;
    std::atomic<bool>       client_connected;
    uint32_t                my_id;
    std::string             my_nick;
    std::vector<GUIPeer>    peers;
    std::mutex              peers_mutex;
    std::atomic<bool>       reconnect_running;

    // ── Live server state ─────────────────────────────────────
    std::shared_ptr<Server>     server;
    std::atomic<bool>           server_running;
    std::vector<GUIClientRow>   server_clients;
    std::mutex                  server_clients_mutex;

    // ── File-send dialog ──────────────────────────────────────
    char sf_filepath[512];
    int  sf_target_id;
    bool sf_to_server;
    bool sf_dialog_open;

    // ── Security ──────────────────────────────────────────────
    SecurityState sec;

    // ── Ctor / dtor ───────────────────────────────────────────
    AppState();
    ~AppState();

    // ── Core helpers ──────────────────────────────────────────
    void push_message(const std::string& sender, const std::string& text,
                      bool is_private=false, bool is_system=false,
                      uint32_t from_id=0);
    void set_status(const std::string& msg, bool error=false);
    void log(const std::string& msg);

    // ── Config ────────────────────────────────────────────────
    void apply_settings();
    void load_settings_from_config();

    // ── Client actions ────────────────────────────────────────
    bool do_connect();
    void do_disconnect();
    void send_chat(const std::string& text);
    void request_list();
    void change_nick(const std::string& nick);
    void send_file_gui(const std::string& path, bool to_server, uint32_t target_id);

    // ── Server actions ────────────────────────────────────────
    bool do_start_server();
    void do_stop_server();
    void refresh_server_clients();
    void kick_client(uint32_t id, const std::string& reason="");
    void server_send_file(const std::string& path, uint32_t target_id, bool to_all);

private:
    void start_reconnect_loop();
};

// ── Window draw functions ─────────────────────────────────────
void draw_connect_window(AppState& app);
void draw_chat_window(AppState& app);
void draw_server_window(AppState& app);
void draw_files_window(AppState& app);
void draw_settings_window(AppState& app);
void draw_navbar(AppState& app);
void apply_dark_theme();
void apply_light_theme();
