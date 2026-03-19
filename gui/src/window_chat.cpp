/**
 * window_chat.cpp — Client chat window with ALL CLI client commands as GUI.
 *
 * CLI commands implemented:
 *   plain text   → broadcast to all
 *   /broadcast   → same as plain text
 *   /msg <id>    → private message by ID
 *   /msgn <nick> → private message by nickname
 *   /list        → request client list refresh
 *   /myid        → show my ID and nick
 *   /nick <n>    → change nickname
 *   /sendfile <id> <path>      → send file to client
 *   /sendfileserver <path>     → send file to server
 *   /config      → show current config
 *   /confighelp  → show config key help
 *   /set <k> <v> → set config value
 *   /saveconfig  → save config to file
 *   /loadconfig  → load config from file
 *   /clear       → clear chat window
 *   /disconnect  → disconnect from server
 *   /help        → show help
 */
#include "../include/app.hpp"
#include "imgui.h"
#include <cstring>
#include <string>
#include <sstream>
#include <mutex>
#include <algorithm>

// ── Colour helpers ────────────────────────────────────────────
static ImVec4 col_system()  { return ImVec4(0.50f,0.50f,0.52f,1.0f); }
static ImVec4 col_pm()      { return ImVec4(0.85f,0.55f,0.85f,1.0f); }
static ImVec4 col_mine()    { return ImVec4(0.88f,0.88f,0.60f,1.0f); }
static ImVec4 col_other()   { return ImVec4(0.60f,0.85f,0.88f,1.0f); }
static ImVec4 col_ts()      { return ImVec4(0.35f,0.35f,0.37f,1.0f); }
static ImVec4 col_accent()  { return ImVec4(0.85f,0.60f,0.15f,1.0f); }
static ImVec4 col_warn()    { return ImVec4(0.88f,0.70f,0.20f,1.0f); }
static ImVec4 col_err()     { return ImVec4(0.88f,0.40f,0.36f,1.0f); }
static ImVec4 col_ok()      { return ImVec4(0.45f,0.82f,0.45f,1.0f); }

static void hsep() {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
}
static void section_hdr(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text, col_accent());
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
    hsep();
    ImGui::Spacing();
}

// ── Config modal state ────────────────────────────────────────
static bool s_show_config     = false;
static bool s_show_confighelp = false;

static void draw_config_modal(AppState& app) {
    if (s_show_config) {
        ImGui::OpenPopup("Config##ccfg");
        s_show_config = false;
    }
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr, ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(580, 540), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Config##ccfg", nullptr,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        section_hdr("CURRENT CONFIGURATION");

        // All config keys and their current values
        struct KV { const char* key; };
        static const KV keys[] = {
            {"host"},{"port"},{"nickname"},{"server_name"},{"motd"},
            {"encrypt_type"},{"encrypt_key"},{"require_key"},{"access_key"},
            {"max_clients"},{"recv_timeout"},{"connect_timeout"},
            {"download_dir"},{"auto_accept_files"},{"max_file_size"},
            {"log_to_file"},{"log_file"},{"verbose"},
            {"keepalive"},{"ping_interval"},{"color_output"},{"buffer_size"}
        };

        ImGui::BeginChild("##cfgscroll", ImVec2(-1, -60), true);
        for (auto& kv : keys) {
            std::string val = g_config.get(kv.key);
            ImGui::PushStyleColor(ImGuiCol_Text, col_accent());
            ImGui::Text("  %-22s", kv.key);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", val.c_str());
        }
        ImGui::EndChild();

        ImGui::Spacing();
        // /set key value inline
        static char set_key[64]={};
        static char set_val[256]={};
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("Key##cfgsetk", set_key, sizeof(set_key));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("Value##cfgsetv", set_val, sizeof(set_val));
        ImGui::SameLine();
        if (ImGui::Button("Set##cfgdoset", ImVec2(50,0)) && set_key[0]) {
            g_config.set(set_key, set_val);
            app.log(std::string("/set ") + set_key + " = " + g_config.get(set_key));
            app.push_message("Config",
                std::string(set_key) + " = " + g_config.get(set_key), false, true);
            memset(set_key,0,sizeof(set_key));
            memset(set_val,0,sizeof(set_val));
        }
        ImGui::SameLine();
        if (ImGui::Button("Close##cfgclose", ImVec2(70,0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static void draw_confighelp_modal() {
    if (s_show_confighelp) {
        ImGui::OpenPopup("Config Help##cch");
        s_show_confighelp = false;
    }
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr, ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(580, 500), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Config Help##cch", nullptr,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        section_hdr("CONFIGURATION KEYS");
        struct HelpRow { const char* key; const char* type; const char* def; const char* desc; };
        static const HelpRow rows[] = {
            {"host",              "string",  "127.0.0.1",         "Server IP to connect to / bind on"},
            {"port",              "int",     "9000",              "TCP port"},
            {"nickname",          "string",  "anonymous",         "Your display name"},
            {"server_name",       "string",  "SafeSocket-Server", "Server display name"},
            {"motd",              "string",  "Welcome!",          "Message of the Day"},
            {"encrypt_type",      "enum",    "NONE",              "NONE|XOR|VIGENERE|RC4"},
            {"encrypt_key",       "string",  "",                  "Encryption passphrase"},
            {"require_key",       "bool",    "false",             "Require access key (server)"},
            {"access_key",        "string",  "",                  "Access key for server auth"},
            {"max_clients",       "int",     "64",                "Max simultaneous connections"},
            {"recv_timeout",      "int",     "300",               "Socket receive timeout (sec)"},
            {"connect_timeout",   "int",     "10",                "Client connect timeout (sec)"},
            {"download_dir",      "string",  "./downloads",       "Directory for received files"},
            {"auto_accept_files", "bool",    "false",             "Accept files without prompt"},
            {"max_file_size",     "int",     "0",                 "Max file size bytes (0=unlimited)"},
            {"log_to_file",       "bool",    "false",             "Write log to file"},
            {"log_file",          "string",  "safesocket.log",    "Log file path"},
            {"verbose",           "bool",    "false",             "Extra diagnostic output"},
            {"keepalive",         "bool",    "true",              "Send keepalive pings"},
            {"ping_interval",     "int",     "30",                "Seconds between pings"},
            {"color_output",      "bool",    "true",              "ANSI colour in console"},
            {"buffer_size",       "int",     "4096",              "I/O buffer size hint"},
        };
        ImGui::BeginChild("##chscroll", ImVec2(-1,-50), true);
        ImGui::PushStyleColor(ImGuiCol_Text, col_accent());
        ImGui::Text("  %-22s %-10s %-18s %s", "KEY","TYPE","DEFAULT","DESCRIPTION");
        ImGui::PopStyleColor();
        hsep();
        for (auto& r : rows) {
            ImGui::PushStyleColor(ImGuiCol_Text, col_accent());
            ImGui::Text("  %-22s", r.key);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextDisabled("%-10s %-18s ", r.type, r.def);
            ImGui::SameLine();
            ImGui::TextWrapped("%s", r.desc);
        }
        ImGui::EndChild();
        ImGui::Spacing();
        if (ImGui::Button("Close##cchcls", ImVec2(-1,28)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ── Process a /command typed into the chat input ──────────────
static void handle_client_command(AppState& app, const std::string& raw,
                                  char* nick_buf, size_t nick_buf_sz)
{
    // Tokenise: first word = cmd, rest = args
    std::istringstream iss(raw);
    std::string cmd;
    iss >> cmd;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    // Helper: get the rest of the stream, trimmed
    auto rest = [&]() -> std::string {
        std::string s;
        std::getline(iss, s);
        while (!s.empty() && s[0] == ' ') s = s.substr(1);
        return s;
    };

    // ── /help ─────────────────────────────────────────────────
    if (cmd == "/help" || cmd == "/?") {
        app.push_message("Help",
            "Commands: plain text=broadcast  "
            "/broadcast <msg>  /msg <id> <msg>  /msgn <nick> <msg>  "
            "/list  /myid  /nick <n>  "
            "/sendfile <id> <path>  /sendfileserver <path>  "
            "/config  /confighelp  /set <k> <v>  /saveconfig [f]  /loadconfig [f]  "
            "/clear  /disconnect",
            false, true);
    }
    // ── /broadcast ────────────────────────────────────────────
    else if (cmd == "/broadcast") {
        std::string msg = rest();
        if (msg.empty()) { app.push_message("System","Usage: /broadcast <message>",false,true); return; }
        app.send_chat(msg);
    }
    // ── /msg <id> <text> ──────────────────────────────────────
    else if (cmd == "/msg") {
        uint32_t id = 0;
        iss >> id;
        std::string msg = rest();
        if (id == 0 || msg.empty()) {
            app.push_message("System","Usage: /msg <id> <message>",false,true); return;
        }
        int old_target = app.chat_pm_target;
        bool old_mode  = app.chat_pm_mode;
        app.chat_pm_target = (int)id;
        app.chat_pm_mode   = true;
        app.send_chat(msg);
        app.chat_pm_target = old_target;
        app.chat_pm_mode   = old_mode;
    }
    // ── /msgn <nick> <text> ───────────────────────────────────
    else if (cmd == "/msgn") {
        std::string nick;
        iss >> nick;
        std::string msg = rest();
        if (nick.empty() || msg.empty()) {
            app.push_message("System","Usage: /msgn <nick> <message>",false,true); return;
        }
        // Find peer by nickname
        uint32_t target_id = 0;
        {
            std::lock_guard<std::mutex> lk(app.peers_mutex);
            for (auto& p : app.peers) {
                if (p.nickname == nick) { target_id = p.id; break; }
            }
        }
        if (target_id == 0) {
            app.push_message("System",
                "Peer '" + nick + "' not found. Try /list first.",false,true);
            return;
        }
        int old_target = app.chat_pm_target;
        bool old_mode  = app.chat_pm_mode;
        app.chat_pm_target = (int)target_id;
        app.chat_pm_mode   = true;
        app.send_chat(msg);
        app.chat_pm_target = old_target;
        app.chat_pm_mode   = old_mode;
    }
    // ── /list ─────────────────────────────────────────────────
    else if (cmd == "/list") {
        app.request_list();
        app.push_message("System","Client list requested.",false,true);
    }
    // ── /myid ─────────────────────────────────────────────────
    else if (cmd == "/myid") {
        app.push_message("System",
            "Your ID: " + std::to_string(app.my_id) +
            "  Nick: " + app.my_nick, false, true);
    }
    // ── /nick <n> ─────────────────────────────────────────────
    else if (cmd == "/nick") {
        std::string nick;
        iss >> nick;
        if (nick.empty()) { app.push_message("System","Usage: /nick <newname>",false,true); return; }
        app.change_nick(nick);
        strncpy(nick_buf, nick.c_str(), nick_buf_sz - 1);
    }
    // ── /sendfile <id> <path> ─────────────────────────────────
    else if (cmd == "/sendfile") {
        uint32_t id = 0;
        iss >> id;
        std::string path = rest();
        if (id == 0 || path.empty()) {
            app.push_message("System","Usage: /sendfile <id> <filepath>",false,true); return;
        }
        app.send_file_gui(path, false, id);
        app.push_message("System","Sending file to #"+std::to_string(id)+": "+path,false,true);
    }
    // ── /sendfileserver <path> ────────────────────────────────
    else if (cmd == "/sendfileserver") {
        std::string path = rest();
        if (path.empty()) {
            app.push_message("System","Usage: /sendfileserver <filepath>",false,true); return;
        }
        app.send_file_gui(path, true, 0);
        app.push_message("System","Sending file to server: "+path,false,true);
    }
    // ── /config ───────────────────────────────────────────────
    else if (cmd == "/config") {
        s_show_config = true;
    }
    // ── /confighelp ───────────────────────────────────────────
    else if (cmd == "/confighelp") {
        s_show_confighelp = true;
    }
    // ── /set <key> <value> ────────────────────────────────────
    else if (cmd == "/set" || cmd == "/setconfig") {
        std::string key;
        iss >> key;
        std::string value = rest();
        if (key.empty()) { app.push_message("System","Usage: /set <key> <value>",false,true); return; }
        g_config.set(key, value);
        app.push_message("Config", key + " = " + g_config.get(key), false, true);
        app.log("/set " + key + " = " + g_config.get(key));
    }
    // ── /saveconfig [path] ────────────────────────────────────
    else if (cmd == "/saveconfig") {
        std::string path; iss >> path;
        if (path.empty()) path = "safesocket.conf";
        if (g_config.save(path))
            app.push_message("Config","Saved to " + path, false, true);
        else
            app.push_message("Config","Save FAILED: " + path, false, true);
    }
    // ── /loadconfig [path] ────────────────────────────────────
    else if (cmd == "/loadconfig") {
        std::string path; iss >> path;
        if (path.empty()) path = "safesocket.conf";
        if (g_config.load(path)) {
            app.push_message("Config","Loaded from " + path, false, true);
            app.load_settings_from_config();
        } else {
            app.push_message("Config","Load FAILED: " + path, false, true);
        }
    }
    // ── /clear ────────────────────────────────────────────────
    else if (cmd == "/clear") {
        std::lock_guard<std::mutex> lk(app.messages_mutex);
        app.messages.clear();
    }
    // ── /disconnect / /quit ───────────────────────────────────
    else if (cmd == "/disconnect" || cmd == "/quit" || cmd == "/exit") {
        app.do_disconnect();
    }
    // ── unknown ───────────────────────────────────────────────
    else {
        app.push_message("System",
            "Unknown command: " + cmd + " (type /help for list)",
            false, true);
    }
}

// ── draw_chat_window ──────────────────────────────────────────
void draw_chat_window(AppState& app)
{
    ImVec2 avail  = ImGui::GetContentRegionAvail();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float  total_h = avail.y;
    float  total_w = avail.x;

    const float sidebar_w = 200.0f;
    const float chat_w    = total_w - sidebar_w - 6.0f;

    // ═══════════════════════════════════════════════════════════
    //  LEFT SIDEBAR — peers + quick actions
    // ═══════════════════════════════════════════════════════════
    ImGui::SetNextWindowPos(ImVec2(origin.x, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(sidebar_w, total_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::Begin("##peers_panel", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    // My identity
    ImGui::PushStyleColor(ImGuiCol_Text, col_accent());
    ImGui::Text(" %s", app.my_nick.empty() ? "anonymous" : app.my_nick.c_str());
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f,0.40f,0.42f,1.0f));
    ImGui::Text(" ID: %u", app.my_id);
    ImGui::PopStyleColor();

    hsep();

    // Nick-change row
    static char nick_buf[64] = {};
    if (nick_buf[0] == '\0')
        strncpy(nick_buf, app.my_nick.c_str(), sizeof(nick_buf)-1);
    ImGui::SetNextItemWidth(sidebar_w - ImGui::GetStyle().WindowPadding.x*2 - 50.0f);
    ImGui::InputText("##nib", nick_buf, sizeof(nick_buf));
    ImGui::SameLine();
    if (ImGui::SmallButton("Set##setnick") && nick_buf[0]) {
        app.change_nick(nick_buf);
    }

    hsep();

    // Refresh
    if (ImGui::SmallButton("Refresh List##listrefresh"))
        app.request_list();

    ImGui::Spacing();

    // Broadcast option
    bool bcast_sel = !app.chat_pm_mode;
    if (bcast_sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.75f,0.52f,0.12f,0.35f));
    if (ImGui::Selectable("[Broadcast All]", bcast_sel)) {
        app.chat_pm_mode   = false;
        app.chat_pm_target = 0;
    }
    if (bcast_sel) ImGui::PopStyleColor();

    // Peer list
    {
        std::lock_guard<std::mutex> lk(app.peers_mutex);
        for (auto& peer : app.peers) {
            bool sel = app.chat_pm_mode && (app.chat_pm_target == (int)peer.id);
            std::string lbl = "[" + std::to_string(peer.id) + "] " + peer.nickname;
            if (peer.id == app.my_id) lbl += " (me)";

            if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.75f,0.52f,0.12f,0.35f));
            if (ImGui::Selectable(lbl.c_str(), sel,
                    ImGuiSelectableFlags_None,
                    ImVec2(sidebar_w - ImGui::GetStyle().WindowPadding.x*2, 0))) {
                if (peer.id != app.my_id) {
                    app.chat_pm_mode   = true;
                    app.chat_pm_target = (int)peer.id;
                }
            }
            if (sel) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Click → PM  ID:%u (%s)", peer.id, peer.nickname.c_str());
        }
        if (app.peers.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f,0.40f,0.42f,1.0f));
            ImGui::TextWrapped("No peers yet.\nHit Refresh.");
            ImGui::PopStyleColor();
        }
    }

    hsep();

    // Quick-action buttons matching CLI
    ImGui::PushStyleColor(ImGuiCol_Text, col_accent());
    ImGui::Text("QUICK ACTIONS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (ImGui::Button("/config", ImVec2(-1,0)))       s_show_config     = true;
    if (ImGui::Button("/confighelp", ImVec2(-1,0)))    s_show_confighelp = true;
    ImGui::Spacing();
    if (ImGui::Button("/myid", ImVec2(-1,0)))
        app.push_message("System",
            "Your ID: " + std::to_string(app.my_id) +
            "  Nick: " + app.my_nick, false, true);
    if (ImGui::Button("/list", ImVec2(-1,0))) {
        app.request_list();
        app.push_message("System","Client list requested.",false,true);
    }
    ImGui::Spacing();
    // /saveconfig and /loadconfig quick buttons
    if (ImGui::Button("Save Config", ImVec2(-1,0))) {
        if (g_config.save("safesocket.conf"))
            app.push_message("Config","Saved to safesocket.conf",false,true);
        else
            app.push_message("Config","Save FAILED",false,true);
    }
    if (ImGui::Button("Load Config", ImVec2(-1,0))) {
        if (g_config.load("safesocket.conf")) {
            app.load_settings_from_config();
            app.push_message("Config","Loaded safesocket.conf",false,true);
        } else {
            app.push_message("Config","Load FAILED",false,true);
        }
    }

    ImGui::End(); // sidebar

    // ═══════════════════════════════════════════════════════════
    //  MAIN CHAT AREA
    // ═══════════════════════════════════════════════════════════
    float chat_x = origin.x + sidebar_w + 6.0f;
    ImGui::SetNextWindowPos(ImVec2(chat_x, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(chat_w, total_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##chat_main", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    // ── Toolbar ─────────────────────────────────────────────────
    if (app.chat_pm_mode && app.chat_pm_target > 0) {
        // Find peer nick for display
        std::string pm_nick;
        {
            std::lock_guard<std::mutex> lk(app.peers_mutex);
            for (auto& p : app.peers)
                if ((int)p.id == app.chat_pm_target) { pm_nick = p.nickname; break; }
        }
        ImGui::PushStyleColor(ImGuiCol_Text, col_pm());
        ImGui::Text("  PM → #%d %s", app.chat_pm_target,
                    pm_nick.empty() ? "" : ("(" + pm_nick + ")").c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::SmallButton("x##clrpm")) {
            app.chat_pm_mode = false; app.chat_pm_target = 0;
        }
        ImGui::SameLine();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.75f,0.85f,1.0f));
        ImGui::Text("  Broadcast");
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }

    ImGui::SetNextItemWidth(90.0f);
    if (ImGui::InputInt("PM→##pmid", &app.chat_pm_target, 0, 0)) {
        app.chat_pm_mode = (app.chat_pm_target > 0);
        if (app.chat_pm_target < 0) app.chat_pm_target = 0;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##clrchat")) {
        std::lock_guard<std::mutex> lk(app.messages_mutex);
        app.messages.clear();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("  /help for commands");

    hsep();

    // ── Message log ─────────────────────────────────────────────
    const float input_h   = 38.0f;
    const float toolbar_h = ImGui::GetItemRectMax().y - origin.y
                            + ImGui::GetStyle().ItemSpacing.y;
    const float log_h     = total_h
                            - toolbar_h
                            - input_h
                            - ImGui::GetStyle().ItemSpacing.y * 3.0f
                            - ImGui::GetStyle().WindowPadding.y * 2.0f
                            - 8.0f;

    ImGui::BeginChild("##msgscroll", ImVec2(0.0f, log_h), false,
        ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard<std::mutex> lk(app.messages_mutex);
        for (auto& msg : app.messages) {
            ImVec4 col;
            if (msg.is_system)
                col = col_system();
            else if (msg.is_private)
                col = col_pm();
            else if (msg.from_id == app.my_id && app.my_id != 0)
                col = col_mine();
            else
                col = col_other();

            ImGui::PushStyleColor(ImGuiCol_Text, col_ts());
            ImGui::Text("[%s]", msg.timestamp.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::Text("%-16s", msg.sender.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.text.c_str());
        }
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.0f)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    // ── Input bar ───────────────────────────────────────────────
    hsep();
    const float send_w  = 72.0f;
    const float input_w = chat_w
                        - send_w
                        - ImGui::GetStyle().ItemSpacing.x
                        - ImGui::GetStyle().WindowPadding.x * 2.0f;

    bool send_triggered = false;
    ImGui::SetNextItemWidth(input_w);
    if (ImGui::InputText("##ci", app.chat_input, sizeof(app.chat_input),
                          ImGuiInputTextFlags_EnterReturnsTrue))
        send_triggered = true;

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f,0.52f,0.12f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f,0.68f,0.20f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.58f,0.38f,0.08f,1.0f));
    bool btn_clicked = ImGui::Button("Send##sendbtn", ImVec2(send_w, 0));
    ImGui::PopStyleColor(3);

    if ((send_triggered || btn_clicked) && app.chat_input[0] != '\0') {
        std::string txt = app.chat_input;
        memset(app.chat_input, 0, sizeof(app.chat_input));

        if (txt[0] == '/') {
            handle_client_command(app, txt, nick_buf, sizeof(nick_buf));
        } else {
            // Plain text → broadcast (same as CLI)
            app.send_chat(txt);
        }
        ImGui::SetKeyboardFocusHere(-1);
    }

    // Draw modals (must be inside same window scope)
    draw_config_modal(app);
    draw_confighelp_modal();

    ImGui::End();
}
