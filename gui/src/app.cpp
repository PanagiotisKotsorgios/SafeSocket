/**
 * @file app.cpp
 * @brief AppState implementation — all GUI↔network bridging.
 *
 * Key design:
 *  - net_init() called once at construction
 *  - Server and Client callbacks route all output into push_message()
 *  - Server screen shown while server running; client screen while connected
 *  - Screen mutex: server running ↔ client connect are mutually exclusive
 */

#include "../include/app.hpp"
#include <cstring>
#include <ctime>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>

#ifdef _WIN32
#  include <direct.h>
#  define mkdir_compat(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define mkdir_compat(p) mkdir(p, 0755)
#endif

// ============================================================
//  ChatMessage ctor
// ============================================================
ChatMessage::ChatMessage(const std::string& s, const std::string& t,
                         bool priv, bool sys, uint32_t fid)
    : sender(s), text(t), is_private(priv), is_system(sys), from_id(fid)
{
    time_t now = time(nullptr);
    char buf[10];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    timestamp = buf;
}

// ============================================================
//  AppState ctor
// ============================================================
AppState::AppState()
    : screen(AppScreen::Connect)
    , status_is_error(false)
    , cs_port(9000)
    , cs_encrypt(0)
    , cs_require_key(false)
    , cs_timeout_sec(10)
    , cs_auto_reconnect(false)
    , sv_port(9000)
    , sv_max_clients(64)
    , sv_require_key(false)
    , sv_encrypt(0)
    , chat_pm_target(0)
    , chat_pm_mode(false)
    , set_auto_accept(false)
    , set_verbose(false)
    , set_log_to_file(false)
    , set_max_file_mb(0)
    , set_ping_interval(30)
    , set_keepalive(true)
    , dark_theme(true)
    , show_about(false)
    , show_help(false)
    , client_connected(false)
    , my_id(0)
    , reconnect_running(false)
    , server_running(false)
    , sf_target_id(0)
    , sf_to_server(true)
    , sf_dialog_open(false)
{
    memset(cs_host,         0, sizeof(cs_host));
    memset(cs_nick,         0, sizeof(cs_nick));
    memset(cs_key,          0, sizeof(cs_key));
    memset(cs_access_key,   0, sizeof(cs_access_key));
    memset(cs_profile_name, 0, sizeof(cs_profile_name));
    memset(sv_host,         0, sizeof(sv_host));
    memset(sv_name,         0, sizeof(sv_name));
    memset(sv_motd,         0, sizeof(sv_motd));
    memset(sv_access_key,   0, sizeof(sv_access_key));
    memset(sv_encrypt_key,  0, sizeof(sv_encrypt_key));
    memset(chat_input,      0, sizeof(chat_input));
    memset(set_download_dir,0, sizeof(set_download_dir));
    memset(set_log_file,    0, sizeof(set_log_file));
    memset(sf_filepath,     0, sizeof(sf_filepath));

    strncpy(cs_host,         "127.0.0.1",             sizeof(cs_host)-1);
    strncpy(cs_nick,         "anonymous",              sizeof(cs_nick)-1);
    strncpy(cs_profile_name, "default",                sizeof(cs_profile_name)-1);
    strncpy(sv_host,         "0.0.0.0",                sizeof(sv_host)-1);
    strncpy(sv_name,         "SafeSocket-Server",      sizeof(sv_name)-1);
    strncpy(sv_motd,         "Welcome to SafeSocket!", sizeof(sv_motd)-1);
    strncpy(set_download_dir,"./downloads",            sizeof(set_download_dir)-1);
    strncpy(set_log_file,    "safesocket.log",         sizeof(set_log_file)-1);

    // Initialize networking subsystem once (WSAStartup on Windows)
    if (!net_init()) {
        log("[net] WARNING: net_init() failed — networking unavailable.");
    }

    load_settings_from_config();
}

// ============================================================
//  AppState dtor — clean shutdown
// ============================================================
AppState::~AppState()
{
    reconnect_running = false;
    if (client_connected && client) {
        client->disconnect();
    }
    if (server_running && server) {
        server->stop();
    }
    net_cleanup();
}

// ============================================================
//  Messaging helpers
// ============================================================
void AppState::push_message(const std::string& sender, const std::string& text,
                             bool is_private, bool is_system, uint32_t from_id)
{
    std::lock_guard<std::mutex> lk(messages_mutex);
    if (messages.size() >= MAX_MESSAGES)
        messages.pop_front();
    messages.push_back(ChatMessage(sender, text, is_private, is_system, from_id));
}

void AppState::set_status(const std::string& msg, bool error)
{
    status_msg      = msg;
    status_is_error = error;
    log(msg);
}

void AppState::log(const std::string& msg)
{
    std::lock_guard<std::mutex> lk(conn_log_mutex);
    if (conn_log.size() >= MAX_LOG)
        conn_log.pop_front();
    conn_log.push_back(msg);
}

// ============================================================
//  Config helpers
// ============================================================
void AppState::load_settings_from_config()
{
    g_config.load("safesocket.conf");

    strncpy(cs_host, g_config.host.c_str(),     sizeof(cs_host)-1);
    cs_port          = g_config.port;
    strncpy(cs_nick, g_config.nickname.c_str(), sizeof(cs_nick)-1);
    cs_timeout_sec   = g_config.connect_timeout;

    switch (g_config.encrypt_type) {
        case EncryptType::NONE:     cs_encrypt = 0; break;
        case EncryptType::XOR:      cs_encrypt = 1; break;
        case EncryptType::VIGENERE: cs_encrypt = 2; break;
        case EncryptType::RC4:      cs_encrypt = 3; break;
    }
    strncpy(cs_key,        g_config.encrypt_key.c_str(),  sizeof(cs_key)-1);
    cs_require_key = g_config.require_key;
    strncpy(cs_access_key, g_config.access_key.c_str(),   sizeof(cs_access_key)-1);

    sv_port        = g_config.port;
    sv_max_clients = g_config.max_clients;
    sv_require_key = g_config.require_key;
    strncpy(sv_name,       g_config.server_name.c_str(),  sizeof(sv_name)-1);
    strncpy(sv_motd,       g_config.motd.c_str(),         sizeof(sv_motd)-1);

    strncpy(set_download_dir, g_config.download_dir.c_str(), sizeof(set_download_dir)-1);
    set_auto_accept   = g_config.auto_accept_files;
    set_verbose       = g_config.verbose;
    set_log_to_file   = g_config.log_to_file;
    strncpy(set_log_file, g_config.log_file.c_str(), sizeof(set_log_file)-1);
    set_max_file_mb   = (int)(g_config.max_file_size / (1024*1024));
    set_ping_interval = g_config.ping_interval;
    set_keepalive     = g_config.keepalive;
}

void AppState::apply_settings()
{
    g_config.download_dir      = set_download_dir;
    g_config.auto_accept_files = set_auto_accept;
    g_config.verbose           = set_verbose;
    g_config.log_to_file       = set_log_to_file;
    g_config.log_file          = set_log_file;
    g_config.max_file_size     = (size_t)set_max_file_mb * 1024 * 1024;
    g_config.ping_interval     = set_ping_interval;
    g_config.keepalive         = set_keepalive;
    mkdir_compat(g_config.download_dir.c_str());
}

// ============================================================
//  CLIENT — connect
// ============================================================
bool AppState::do_connect()
{
    if (client_connected.load()) {
        set_status("Already connected.", true);
        return false;
    }
    if (server_running.load()) {
        set_status("Stop the server before connecting as client.", true);
        return false;
    }

    // Push GUI fields → g_config
    g_config.host            = cs_host;
    g_config.port            = cs_port;
    g_config.nickname        = cs_nick;
    g_config.connect_timeout = cs_timeout_sec;
    g_config.require_key     = cs_require_key;
    g_config.access_key      = cs_require_key ? std::string(cs_access_key) : "";
    g_config.encrypt_key     = cs_key;

    static const EncryptType etab[] = {
        EncryptType::NONE, EncryptType::XOR,
        EncryptType::VIGENERE, EncryptType::RC4
    };
    g_config.encrypt_type = etab[cs_encrypt < 4 ? cs_encrypt : 0];

    apply_settings();
    set_status("Connecting to " + std::string(cs_host) + ":" + std::to_string(cs_port) + " ...");

    client  = std::make_shared<Client>();
    my_nick = cs_nick;

    AppState* self = this;

    // ── Message callback: all Client::print() → push_message ──────
    client->set_message_callback([self](const std::string& msg,
                                         bool is_priv,
                                         bool is_sys)
    {
        // Parse "[sender] text" if present — the server prefixes broadcasts
        std::string sender = "Server";
        std::string text   = msg;

        if (!msg.empty() && msg[0] == '[') {
            size_t rb = msg.find(']');
            if (rb != std::string::npos && rb + 2 <= msg.size()) {
                sender = msg.substr(1, rb - 1);
                text   = msg.substr(rb + 2);
            }
        }
        self->push_message(sender, text, is_priv, is_sys);
    });

    // ── Peer-list callback: refresh sidebar ───────────────────────
    client->set_peer_list_callback([self](
        const std::vector<std::pair<uint32_t,std::string>>& list)
    {
        std::lock_guard<std::mutex> lk(self->peers_mutex);
        self->peers.clear();
        for (auto& p : list)
            self->peers.push_back({p.first, p.second});
    });

    // Connect in background thread
    std::thread([self](){
        bool ok = self->client->connect(g_config.host, g_config.port);
        if (!ok) {
            self->client.reset();
            self->set_status("Connection failed — check host/port and encryption settings.", true);
            self->push_message("System", "Connection failed.", false, true);
            return;
        }

        self->client_connected = true;
        self->my_id   = 0;
        self->screen  = AppScreen::Client;

        self->set_status("Connected to " + g_config.host +
                         ":" + std::to_string(g_config.port) +
                         " as \"" + g_config.nickname + "\"");
        self->push_message("System",
            "Connected to " + g_config.host + ":" +
            std::to_string(g_config.port) +
            " as \"" + g_config.nickname + "\"",
            false, true);

        // Request initial client list
        self->client->request_client_list();

        // Poll until disconnected
        while (self->client && self->client->is_connected()) {
            // Sync assigned ID
            if (self->my_id == 0 && self->client->my_id() != 0)
                self->my_id = self->client->my_id();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Disconnected
        if (self->client_connected.load()) {
            self->client_connected = false;
            self->push_message("System", "Connection closed by server.", false, true);
            self->set_status("Disconnected.", true);
            {
                std::lock_guard<std::mutex> lk(self->peers_mutex);
                self->peers.clear();
            }
            self->my_id = 0;
            self->screen = AppScreen::Connect;
        }

        // Auto-reconnect
        if (self->cs_auto_reconnect && !self->server_running.load()) {
            self->start_reconnect_loop();
        }
    }).detach();

    return true;
}

// ============================================================
//  CLIENT — disconnect
// ============================================================
void AppState::do_disconnect()
{
    reconnect_running = false;

    if (!client_connected.load() || !client) return;

    client_connected = false;
    client->disconnect();
    client.reset();

    {
        std::lock_guard<std::mutex> lk(peers_mutex);
        peers.clear();
    }
    my_id  = 0;
    screen = AppScreen::Connect;
    set_status("Disconnected.");
    push_message("System", "Disconnected from server.", false, true);
}

// ============================================================
//  CLIENT — auto-reconnect
// ============================================================
void AppState::start_reconnect_loop()
{
    if (reconnect_running.load()) return;
    reconnect_running = true;

    AppState* self = this;
    std::thread([self](){
        int attempt = 0;
        while (self->reconnect_running.load() && !self->client_connected.load()) {
            ++attempt;
            self->log("Auto-reconnect attempt #" + std::to_string(attempt) +
                      " in 5 s...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!self->reconnect_running.load()) break;
            self->do_connect();
            // Wait up to 6 s for connect to succeed
            for (int i = 0; i < 60 && !self->client_connected.load(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (self->client_connected.load()) {
                self->reconnect_running = false;
                break;
            }
        }
    }).detach();
}

// ============================================================
//  CLIENT — send chat / PM
// ============================================================
void AppState::send_chat(const std::string& text)
{
    if (!client || !client_connected.load()) {
        set_status("Not connected.", true);
        return;
    }
    if (text.empty()) return;

    if (chat_pm_mode && chat_pm_target > 0) {
        // Private message — send raw text; server will wrap it
        client->send_private((uint32_t)chat_pm_target, text);
        push_message("[PM → #" + std::to_string(chat_pm_target) + "]",
                     text, true, false, my_id);
    } else {
        // Broadcast — Client::send_broadcast wraps as "[nick] text" internally
        // so we pass raw text; push our own copy for display
        client->send_broadcast(text);
        push_message(my_nick.empty() ? "Me" : my_nick,
                     text, false, false, my_id);
    }
}

// ============================================================
//  CLIENT — request list
// ============================================================
void AppState::request_list()
{
    if (client && client_connected.load())
        client->request_client_list();
}

// ============================================================
//  CLIENT — nick change (proper MSG_NICK_SET packet)
// ============================================================
void AppState::change_nick(const std::string& nick)
{
    if (nick.empty()) return;
    my_nick           = nick;
    g_config.nickname = nick;

    if (client && client_connected.load()) {
        // Send MSG_NICK_SET packet directly through the socket
        // Client doesn't expose this — use send_broadcast with /nick command
        // The server parses MSG_NICK_SET; we need to emit it properly.
        // We add a direct send method via the patched client.
        client->send_nick_change(nick);
        push_message("System", "Nickname changed to: " + nick, false, true);
    }
    log("Nick → " + nick);
}

// ============================================================
//  CLIENT — send file
// ============================================================
void AppState::send_file_gui(const std::string& path,
                              bool to_server, uint32_t target_id)
{
    if (!client || !client_connected.load()) {
        set_status("Not connected.", true);
        return;
    }

    std::string fname = path;
    size_t pos = fname.find_last_of("/\\");
    if (pos != std::string::npos) fname = fname.substr(pos + 1);

    FileTransferEntry entry;
    entry.filename   = fname;
    entry.is_upload  = true;
    entry.peer       = to_server ? "Server"
                                 : ("Client #" + std::to_string(target_id));
    entry.progress   = 0.0f;

    size_t idx;
    {
        std::lock_guard<std::mutex> lk(transfers_mutex);
        transfers.push_back(entry);
        idx = transfers.size() - 1;
    }

    AppState* self = this;
    std::thread([self, path, to_server, target_id, idx](){
        bool ok = to_server
            ? self->client->send_file_to_server(path)
            : self->client->send_file_to_client(target_id, path);

        {
            std::lock_guard<std::mutex> lk(self->transfers_mutex);
            if (idx < self->transfers.size()) {
                self->transfers[idx].done     = ok;
                self->transfers[idx].failed   = !ok;
                self->transfers[idx].progress = ok ? 1.0f : 0.0f;
            }
        }
        if (ok)  self->log("File sent: " + path);
        else     self->log("File send FAILED: " + path);
        self->push_message("System",
            ok ? ("File sent: " + path) : ("File send FAILED: " + path),
            false, true);
    }).detach();
}

// ============================================================
//  SERVER — start
// ============================================================
bool AppState::do_start_server()
{
    if (server_running.load()) {
        set_status("Server already running.", true);
        return false;
    }
    if (client_connected.load()) {
        set_status("Disconnect the client first.", true);
        return false;
    }

    // Push server GUI fields → g_config
    g_config.host        = sv_host;
    g_config.port        = sv_port;
    g_config.server_name = sv_name;
    g_config.motd        = sv_motd;
    g_config.max_clients = sv_max_clients;
    g_config.require_key = sv_require_key;
    g_config.access_key  = sv_require_key ? std::string(sv_access_key) : "";

    static const EncryptType etab[] = {
        EncryptType::NONE, EncryptType::XOR,
        EncryptType::VIGENERE, EncryptType::RC4
    };
    g_config.encrypt_type = etab[sv_encrypt < 4 ? sv_encrypt : 0];
    g_config.encrypt_key  = sv_encrypt_key;

    apply_settings();
    set_status("Starting server on " + std::string(sv_host) +
               ":" + std::to_string(sv_port) + " ...");

    server = std::make_shared<Server>();

    AppState* self = this;

    // ── Server message callback → push_message ─────────────────────
    server->set_message_callback([self](const std::string& msg, bool is_err){
        // Parse "[server] Client X identified as [nick]" etc.
        std::string sender = "Server";
        std::string text   = msg;

        // Strip common server prefixes for cleaner display
        if (msg.substr(0, 9) == "[server] ") {
            text = msg.substr(9);
        } else if (msg.substr(0, 8) == "[SERVER]") {
            text = msg.substr(8);
            if (!text.empty() && text[0] == ' ') text = text.substr(1);
        }
        self->push_message(sender, text, false, true);
        if (is_err) self->set_status(text, true);
    });

    // Start server in background thread
    std::thread([self](){
        bool ok = self->server->start(g_config.host, g_config.port);
        if (!ok) {
            self->server.reset();
            self->set_status("Server failed to start — port may be in use.", true);
            self->push_message("System", "Server failed to start.", false, true);
            return;
        }

        self->server_running = true;
        self->screen         = AppScreen::Server;
        self->set_status("Server running on " + g_config.host +
                         ":" + std::to_string(g_config.port));
        self->push_message("System",
            "Server started on " + g_config.host +
            ":" + std::to_string(g_config.port) +
            "  enc:" + encrypt_type_name(g_config.encrypt_type),
            false, true);

        // Refresh client list every second while running
        while (self->server_running.load() && self->server) {
            self->refresh_server_clients();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();

    return true;
}

// ============================================================
//  SERVER — stop
// ============================================================
void AppState::do_stop_server()
{
    if (!server_running.load() || !server) return;

    server_running = false;
    server->stop();
    server.reset();

    {
        std::lock_guard<std::mutex> lk(server_clients_mutex);
        server_clients.clear();
    }
    screen = AppScreen::Connect;
    set_status("Server stopped.");
    push_message("System", "Server stopped.", false, true);
}

// ============================================================
//  SERVER — refresh client snapshot
// ============================================================
void AppState::refresh_server_clients()
{
    if (!server) return;
    auto snap = server->get_clients_snapshot();

    std::lock_guard<std::mutex> lk(server_clients_mutex);
    server_clients.clear();
    for (auto& s : snap) {
        GUIClientRow row;
        row.id            = s.id;
        row.nickname      = s.nickname;
        row.ip            = s.ip;
        row.authenticated = s.authenticated;
        char buf[32];
        struct tm* tm_info = localtime(&s.connected_at);
        strftime(buf, sizeof(buf), "%H:%M:%S", tm_info);
        row.connected_at  = buf;
        server_clients.push_back(row);
    }
}

// ============================================================
//  SERVER — kick client
// ============================================================
void AppState::kick_client(uint32_t id, const std::string& reason)
{
    if (!server) return;
    server->kick_client(id, reason);
    log("Kicked client #" + std::to_string(id) +
        (reason.empty() ? "" : " — " + reason));
    push_message("System",
        "Kicked #" + std::to_string(id) +
        (reason.empty() ? "" : " — " + reason),
        false, true);
}

// ============================================================
//  SERVER — send file to specific client
// ============================================================
void AppState::server_send_file(const std::string& path, uint32_t target_id, bool to_all)
{
    if (!server || !server_running.load()) {
        set_status("Server not running.", true);
        return;
    }

    std::string fname = path;
    size_t p = fname.find_last_of("/\\");
    if (p != std::string::npos) fname = fname.substr(p + 1);

    FileTransferEntry entry;
    entry.filename   = fname;
    entry.is_upload  = true;
    entry.peer       = to_all ? "All clients"
                              : ("Client #" + std::to_string(target_id));
    entry.progress   = 0.0f;
    size_t idx;
    {
        std::lock_guard<std::mutex> lk(transfers_mutex);
        transfers.push_back(entry);
        idx = transfers.size() - 1;
    }

    AppState* self = this;
    std::thread([self, path, target_id, to_all, idx](){
        bool ok = false;
        if (to_all)
            ok = self->server->send_file_all(path);
        else
            ok = self->server->send_file(target_id, path);

        {
            std::lock_guard<std::mutex> lk(self->transfers_mutex);
            if (idx < self->transfers.size()) {
                self->transfers[idx].done     = ok;
                self->transfers[idx].failed   = !ok;
                self->transfers[idx].progress = ok ? 1.0f : 0.0f;
            }
        }
        self->log("Server file send " + std::string(ok?"OK":"FAILED") + ": " + path);
        self->push_message("System",
            std::string("Server file send ") + (ok?"OK":"FAILED") + ": " + path,
            false, true);
    }).detach();
}
