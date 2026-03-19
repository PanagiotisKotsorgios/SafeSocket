/**
 * window_server.cpp — Server admin panel with ALL CLI server commands as GUI.
 *
 * CLI commands implemented:
 *   /list                     → client table (auto-refreshed)
 *   /stats                    → stats panel
 *   /broadcast <msg>          → broadcast input
 *   /msg <id> <msg>           → PM by ID
 *   /msgn <nick> <msg>        → PM by nickname
 *   /kick <id> [reason]       → kick by ID
 *   /kickn <nick> [reason]    → kick by nickname
 *   /sendfile <id> <path>     → send file to one client
 *   /sendfileall <path>       → send file to all clients
 *   /config                   → show config
 *   /confighelp               → config key help
 *   /set <k> <v>              → set config value
 *   /saveconfig [file]        → save config
 *   /loadconfig [file]        → load config
 *   /stop                     → stop server
 */
#include "../include/app.hpp"
#include "imgui.h"
#include <cstring>
#include <string>
#include <sstream>
#include <mutex>
#include <cstdio>
#include <algorithm>

static void hsep_s() {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
}
static void acl(const char* s) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.60f,0.15f,1.0f));
    ImGui::Text("%s", s); ImGui::PopStyleColor();
}

// ── Config modal ──────────────────────────────────────────────
static bool s_srv_show_config     = false;
static bool s_srv_show_confighelp = false;

static void draw_srv_config_modal(AppState& app) {
    if (s_srv_show_config) { ImGui::OpenPopup("ServerConfig##sc"); s_srv_show_config=false; }
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(580,540),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("ServerConfig##sc",nullptr,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.60f,0.15f,1.0f));
        ImGui::Text("CURRENT CONFIGURATION");
        ImGui::PopStyleColor();
        hsep_s(); ImGui::Spacing();

        static const char* keys[] = {
            "host","port","server_name","motd","max_clients",
            "encrypt_type","encrypt_key","require_key","access_key",
            "recv_timeout","connect_timeout",
            "download_dir","auto_accept_files","max_file_size",
            "log_to_file","log_file","verbose",
            "keepalive","ping_interval","color_output","buffer_size"
        };
        ImGui::BeginChild("##sccfgscroll",ImVec2(-1,-60),true);
        for (auto k : keys) {
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.60f,0.15f,1.0f));
            ImGui::Text("  %-22s", k);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", g_config.get(k).c_str());
        }
        ImGui::EndChild();
        ImGui::Spacing();

        static char sk[64]={}, sv_val[256]={};
        ImGui::SetNextItemWidth(150); ImGui::InputText("Key##sck",sk,sizeof(sk));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200); ImGui::InputText("Value##scv",sv_val,sizeof(sv_val));
        ImGui::SameLine();
        if (ImGui::Button("Set##scdoset") && sk[0]) {
            g_config.set(sk,sv_val);
            app.log(std::string("/set ")+sk+" = "+g_config.get(sk));
            app.push_message("Config",std::string(sk)+" = "+g_config.get(sk),false,true);
            memset(sk,0,sizeof(sk)); memset(sv_val,0,sizeof(sv_val));
        }
        ImGui::SameLine();
        if (ImGui::Button("Close##scclose")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static void draw_srv_confighelp_modal() {
    if (s_srv_show_confighelp) { ImGui::OpenPopup("SrvConfigHelp##sch"); s_srv_show_confighelp=false; }
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(580,480),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("SrvConfigHelp##sch",nullptr,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.60f,0.15f,1.0f));
        ImGui::Text("CONFIGURATION KEYS");
        ImGui::PopStyleColor(); hsep_s(); ImGui::Spacing();
        struct HRow{const char*key,*type,*def,*desc;};
        static const HRow rows[]={
            {"host","string","127.0.0.1","Server bind address"},
            {"port","int","9000","TCP port"},
            {"server_name","string","SafeSocket-Server","Server display name"},
            {"motd","string","Welcome!","Message of the Day"},
            {"max_clients","int","64","Max simultaneous connections"},
            {"encrypt_type","enum","NONE","NONE|XOR|VIGENERE|RC4"},
            {"encrypt_key","string","","Encryption passphrase"},
            {"require_key","bool","false","Require access key"},
            {"access_key","string","","Access key for auth"},
            {"recv_timeout","int","300","Socket receive timeout (sec)"},
            {"connect_timeout","int","10","Client connect timeout (sec)"},
            {"download_dir","string","./downloads","Dir for received files"},
            {"auto_accept_files","bool","false","Accept files without prompt"},
            {"max_file_size","int","0","Max file size bytes (0=unlimited)"},
            {"log_to_file","bool","false","Write log to file"},
            {"log_file","string","safesocket.log","Log file path"},
            {"verbose","bool","false","Extra diagnostic output"},
            {"keepalive","bool","true","Send keepalive pings"},
            {"ping_interval","int","30","Seconds between pings"},
            {"color_output","bool","true","ANSI colour in console"},
            {"buffer_size","int","4096","I/O buffer size hint"},
        };
        ImGui::BeginChild("##schscroll",ImVec2(-1,-50),true);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.60f,0.15f,1.0f));
        ImGui::Text("  %-22s %-10s %-18s %s","KEY","TYPE","DEFAULT","DESCRIPTION");
        ImGui::PopStyleColor(); hsep_s();
        for (auto& r:rows){
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.60f,0.15f,1.0f));
            ImGui::Text("  %-22s",r.key); ImGui::PopStyleColor();
            ImGui::SameLine(); ImGui::TextDisabled("%-10s %-18s ",r.type,r.def);
            ImGui::SameLine(); ImGui::TextWrapped("%s",r.desc);
        }
        ImGui::EndChild();
        ImGui::Spacing();
        if (ImGui::Button("Close##schcls",ImVec2(-1,28))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ── draw_server_window ────────────────────────────────────────
void draw_server_window(AppState& app)
{
    ImVec2 avail  = ImGui::GetContentRegionAvail();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float  total_h = avail.y;
    float  total_w = avail.x;

    float left_w  = total_w * 0.60f;
    float right_w = total_w - left_w - 6.0f;

    // ═══════════════════════════════════════════════════════════
    //  LEFT — client table (/list equivalent)
    // ═══════════════════════════════════════════════════════════
    ImGui::SetNextWindowPos(ImVec2(origin.x, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(left_w, total_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##srv_clients", NULL,
        ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(1.05f);
    ImGui::Text("Connected Clients  (/list)");
    ImGui::SetWindowFontScale(1.0f);

    // Stats row (/stats equivalent)
    if (app.server) {
        auto s = app.server->get_stats();
        ImGui::SameLine();
        ImGui::TextDisabled("  %zu/%d  msgs:%llu  out:%llu B  in:%llu B  enc:%s",
            s.clients_online, g_config.max_clients,
            (unsigned long long)s.msg_count,
            (unsigned long long)s.bytes_sent,
            (unsigned long long)s.bytes_recv,
            s.encrypt_type.c_str());
    }
    hsep_s();

    // Column header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.60f,0.15f,1.0f));
    ImGui::Text(" %-6s %-22s %-18s %-5s %s",
        "ID","Nickname","IP","Auth","Time");
    ImGui::PopStyleColor();
    hsep_s();

    static int  selected_id = -1;
    static char kick_reason[256] = "Kicked by admin";
    static char srv_pm_buf[512]  = {};
    static char srv_file_buf[512]= {};

    float ctrl_h = 120.0f;
    ImGui::BeginChild("##clilist", ImVec2(0, -ctrl_h), false);
    {
        std::lock_guard<std::mutex> lk(app.server_clients_mutex);
        for (auto& row : app.server_clients) {
            bool sel = (selected_id == (int)row.id);
            char line[256];
            snprintf(line,sizeof(line)," %-6u %-22s %-18s %-5s %s",
                row.id, row.nickname.substr(0,22).c_str(),
                row.ip.substr(0,18).c_str(),
                row.authenticated?"Yes":"No",
                row.connected_at.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text,
                row.authenticated
                    ? ImVec4(0.88f,0.88f,0.86f,1.0f)
                    : ImVec4(0.75f,0.55f,0.30f,1.0f));
            if (ImGui::Selectable(line, sel, ImGuiSelectableFlags_SpanAllColumns))
                selected_id = (int)row.id;
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("ID:%u  Nick:%s  IP:%s",
                    row.id,row.nickname.c_str(),row.ip.c_str());
        }
        if (app.server_clients.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.42f,0.42f,0.44f,1.0f));
            ImGui::Text("  No clients connected.");
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
    hsep_s();

    // ── Per-client actions ────────────────────────────────────
    bool can_act = (selected_id > 0);
    if (!can_act) ImGui::BeginDisabled();

    // /kick <id> [reason]
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputText("Reason##kr", kick_reason, sizeof(kick_reason));
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f,0.20f,0.18f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f,0.30f,0.28f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f,0.14f,0.12f,1.0f));
    if (ImGui::Button("/kick Selected##kickbtn", ImVec2(120,0))) {
        app.kick_client((uint32_t)selected_id, kick_reason);
        selected_id = -1;
    }
    ImGui::PopStyleColor(3);

    // /msg <id> <text>
    ImGui::SetNextItemWidth(200.0f);
    bool pm_enter = ImGui::InputText("PM msg##srvpm", srv_pm_buf, sizeof(srv_pm_buf),
                                     ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((pm_enter || ImGui::Button("/msg##spmbtn", ImVec2(70,0))) && srv_pm_buf[0]) {
        if (app.server) {
            app.server->send_text((uint32_t)selected_id,
                                  std::string("[SERVER→PM] ") + srv_pm_buf, SERVER_ID);
            app.push_message("[PM→#"+std::to_string(selected_id)+"]",
                             srv_pm_buf, true, false, 0);
            app.log("/msg "+std::to_string(selected_id)+" "+srv_pm_buf);
        }
        memset(srv_pm_buf,0,sizeof(srv_pm_buf));
    }

    // /sendfile <id> <path>
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputText("File##srvfile", srv_file_buf, sizeof(srv_file_buf));
    ImGui::SameLine();
    if (ImGui::Button("/sendfile##sfbtn", ImVec2(90,0)) && srv_file_buf[0]) {
        app.server_send_file(srv_file_buf, (uint32_t)selected_id, false);
        memset(srv_file_buf,0,sizeof(srv_file_buf));
    }

    if (!can_act) {
        ImGui::EndDisabled();
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.42f,0.42f,0.44f,1.0f));
        ImGui::Text("   ← select a client first");
        ImGui::PopStyleColor();
    }

    ImGui::End(); // left panel

    // ═══════════════════════════════════════════════════════════
    //  RIGHT — admin controls + command panel + log
    // ═══════════════════════════════════════════════════════════
    float rx = origin.x + left_w + 6.0f;
    ImGui::SetNextWindowPos(ImVec2(rx, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(right_w, total_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##srv_right", NULL,
        ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    // Server info (/stats)
    ImGui::SetWindowFontScale(1.05f); ImGui::Text("Server  (/stats)"); ImGui::SetWindowFontScale(1.0f);
    hsep_s();
    {
        auto row = [](const char* k, const char* v){
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.60f,0.15f,1.0f));
            ImGui::Text("%-14s",k); ImGui::PopStyleColor();
            ImGui::SameLine(); ImGui::Text("%s",v);
        };
        row("Name:", g_config.server_name.c_str());
        row("Address:", (g_config.host+":"+std::to_string(g_config.port)).c_str());
        row("Encryption:", encrypt_type_name(g_config.encrypt_type).c_str());
        row("Auth:", g_config.require_key?"Key required":"Open");
        if (app.server) {
            auto st = app.server->get_stats();
            row("Clients:",  (std::to_string(st.clients_online)+"/"+std::to_string(g_config.max_clients)).c_str());
            row("Messages:", std::to_string(st.msg_count).c_str());
            row("Bytes out:", fmt_bytes(st.bytes_sent).c_str());
            row("Bytes in:",  fmt_bytes(st.bytes_recv).c_str());
        }
    }
    ImGui::Spacing();
    hsep_s();
    ImGui::Spacing();

    // ── /broadcast ────────────────────────────────────────────
    acl("/broadcast");
    static char bcast_buf[512]={};
    bool bcast_enter = ImGui::InputText("##bcast", bcast_buf, sizeof(bcast_buf),
                                        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((bcast_enter||ImGui::Button("Send##bcastsend",ImVec2(-1,0))) && bcast_buf[0]) {
        if (app.server) {
            app.server->broadcast_text(std::string("[SERVER] ")+bcast_buf, SERVER_ID);
            app.push_message("Admin", bcast_buf, false, false, 0);
            app.log("/broadcast "+std::string(bcast_buf));
        }
        memset(bcast_buf,0,sizeof(bcast_buf));
    }
    ImGui::Spacing();

    // ── /msgn <nick> <msg> ────────────────────────────────────
    acl("/msgn <nick>");
    static char msgn_nick[64]={}, msgn_text[512]={};
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputText("Nick##msgnick",  msgn_nick, sizeof(msgn_nick));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(right_w - 220.0f);
    bool msgn_enter = ImGui::InputText("Msg##msgnmsg", msgn_text, sizeof(msgn_text),
                                       ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((msgn_enter||ImGui::Button("Send##msgnbtn",ImVec2(-1,0)))
        && msgn_nick[0] && msgn_text[0])
    {
        if (app.server) {
            uint32_t tid = app.server->find_client_by_nick(msgn_nick);
            if (tid != 0) {
                app.server->send_text(tid,
                    std::string("[SERVER→PM] ")+msgn_text, SERVER_ID);
                app.push_message("[PM→"+std::string(msgn_nick)+"]",
                                 msgn_text, true, false, 0);
                app.log("/msgn "+std::string(msgn_nick)+" "+msgn_text);
            } else {
                app.push_message("System",
                    "Client '"+std::string(msgn_nick)+"' not found.",false,true);
            }
        }
        memset(msgn_text,0,sizeof(msgn_text));
    }
    ImGui::Spacing();

    // ── /kickn <nick> [reason] ────────────────────────────────
    acl("/kickn <nick>");
    static char kickn_nick[64]={}, kickn_reason[256]="Kicked by admin";
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("Nick##kcknick", kickn_nick, sizeof(kickn_nick));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(right_w - 280.0f);
    ImGui::InputText("Reason##kckrn", kickn_reason, sizeof(kickn_reason));
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f,0.20f,0.18f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f,0.30f,0.28f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f,0.14f,0.12f,1.0f));
    if (ImGui::Button("/kickn##kicknbtn",ImVec2(-1,0)) && kickn_nick[0]) {
        if (app.server) {
            uint32_t tid = app.server->find_client_by_nick(kickn_nick);
            if (tid != 0) {
                app.kick_client(tid, kickn_reason);
                memset(kickn_nick,0,sizeof(kickn_nick));
            } else {
                app.push_message("System",
                    "Client '"+std::string(kickn_nick)+"' not found.",false,true);
            }
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::Spacing();

    // ── /sendfileall <path> ───────────────────────────────────
    acl("/sendfileall");
    static char sfall_buf[512]={};
    ImGui::SetNextItemWidth(-80.0f);
    ImGui::InputText("##sfall", sfall_buf, sizeof(sfall_buf));
    ImGui::SameLine();
    if (ImGui::Button("Send All##sfallbtn",ImVec2(-1,0)) && sfall_buf[0]) {
        app.server_send_file(sfall_buf, 0, true);
        memset(sfall_buf,0,sizeof(sfall_buf));
    }
    ImGui::Spacing();

    // ── Config actions ────────────────────────────────────────
    acl("CONFIG");
    if (ImGui::Button("/config##scbtn",     ImVec2(-1,0))) s_srv_show_config     = true;
    if (ImGui::Button("/confighelp##schbtn",ImVec2(-1,0))) s_srv_show_confighelp = true;

    // /set <key> <value>
    static char sc_key[64]={}, sc_val[256]={};
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("K##sck2",sc_key,sizeof(sc_key));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(right_w-240.0f);
    bool sc_enter = ImGui::InputText("V##scv2",sc_val,sizeof(sc_val),
                                     ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((sc_enter||ImGui::Button("/set##scdoset2",ImVec2(-1,0))) && sc_key[0]) {
        g_config.set(sc_key,sc_val);
        app.push_message("Config",std::string(sc_key)+" = "+g_config.get(sc_key),false,true);
        app.log("/set "+std::string(sc_key)+" = "+g_config.get(sc_key));
        memset(sc_key,0,sizeof(sc_key)); memset(sc_val,0,sizeof(sc_val));
    }

    if (ImGui::Button("Save safesocket.conf##svscfg",ImVec2(-1,0))) {
        if (g_config.save("safesocket.conf"))
            app.push_message("Config","Saved to safesocket.conf",false,true);
        else
            app.push_message("Config","Save FAILED",false,true);
    }
    if (ImGui::Button("Load safesocket.conf##ldscfg",ImVec2(-1,0))) {
        if (g_config.load("safesocket.conf")) {
            app.load_settings_from_config();
            app.push_message("Config","Loaded safesocket.conf",false,true);
        } else {
            app.push_message("Config","Load FAILED",false,true);
        }
    }

    ImGui::Spacing(); hsep_s(); ImGui::Spacing();

    // ── Stop server ───────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f,0.20f,0.18f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f,0.30f,0.28f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f,0.14f,0.12f,1.0f));
    if (ImGui::Button("/stop Server", ImVec2(-1.0f, 34.0f)))
        app.do_stop_server();
    ImGui::PopStyleColor(3);

    ImGui::Spacing(); hsep_s();

    // ── Log ───────────────────────────────────────────────────
    acl("LOG");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 52.0f);
    if (ImGui::SmallButton("Clear##srvlogclr")) {
        std::lock_guard<std::mutex> lk(app.conn_log_mutex);
        app.conn_log.clear();
    }
    ImGui::BeginChild("##srvlog",ImVec2(0,-1),false,ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard<std::mutex> lk(app.conn_log_mutex);
        for (size_t i=0;i<app.conn_log.size();++i) {
            const std::string& e=app.conn_log[i];
            ImVec4 col=ImVec4(0.54f,0.54f,0.56f,1.0f);
            if (e.find("error")!=e.npos||e.find("fail")!=e.npos||e.find("FAIL")!=e.npos)
                col=ImVec4(0.88f,0.40f,0.36f,1.0f);
            else if (e.find("connect")!=e.npos||e.find("start")!=e.npos||e.find("OK")!=e.npos)
                col=ImVec4(0.55f,0.82f,0.55f,1.0f);
            else if (e.find("Kicked")!=e.npos||e.find("KICKED")!=e.npos)
                col=ImVec4(0.85f,0.55f,0.30f,1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text,col);
            ImGui::TextWrapped("[%03d] %s",(int)(i+1),e.c_str());
            ImGui::PopStyleColor();
        }
    }
    if (ImGui::GetScrollY()>=ImGui::GetScrollMaxY()-4.0f) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    // Modals
    draw_srv_config_modal(app);
    draw_srv_confighelp_modal();

    ImGui::End();
}
