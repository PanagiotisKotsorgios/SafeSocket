#include "../include/app.hpp"
#include "imgui.h"
#include <cstring>
#include <cstdio>
#include <string>

static void save_profile(const AppState& app)
{
    FILE* f = fopen("safesocket.profile", "w");
    if (!f) return;
    fprintf(f, "host=%s\nport=%d\nnick=%s\nencrypt=%d\ntimeout=%d\nautorecon=%d\n",
            app.cs_host, app.cs_port, app.cs_nick,
            app.cs_encrypt, app.cs_timeout_sec, app.cs_auto_reconnect ? 1 : 0);
    fprintf(f, "requirekey=%d\nkey=%s\naccess_key=%s\n",
            app.cs_require_key ? 1 : 0, app.cs_key, app.cs_access_key);
    fprintf(f, "sv_host=%s\nsv_port=%d\nsv_name=%s\nsv_motd=%s\n",
            app.sv_host, app.sv_port, app.sv_name, app.sv_motd);
    fprintf(f, "sv_max=%d\nsv_reqkey=%d\nsv_access=%s\nsv_encrypt=%d\nsv_ekey=%s\n",
            app.sv_max_clients, app.sv_require_key ? 1 : 0,
            app.sv_access_key, app.sv_encrypt, app.sv_encrypt_key);
    fclose(f);
}

static void load_profile(AppState& app)
{
    FILE* f = fopen("safesocket.profile", "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char key[64]={}, val[448]={};
        if (sscanf(line, "%63[^=]=%447[^\n]", key, val) != 2) continue;
        if (!strcmp(key,"host"))       strncpy(app.cs_host,       val, sizeof(app.cs_host)-1);
        if (!strcmp(key,"nick"))       strncpy(app.cs_nick,       val, sizeof(app.cs_nick)-1);
        if (!strcmp(key,"port"))       app.cs_port           = atoi(val);
        if (!strcmp(key,"encrypt"))    app.cs_encrypt        = atoi(val);
        if (!strcmp(key,"timeout"))    app.cs_timeout_sec    = atoi(val);
        if (!strcmp(key,"autorecon"))  app.cs_auto_reconnect = atoi(val)!=0;
        if (!strcmp(key,"requirekey")) app.cs_require_key    = atoi(val)!=0;
        if (!strcmp(key,"key"))        strncpy(app.cs_key,        val, sizeof(app.cs_key)-1);
        if (!strcmp(key,"access_key")) strncpy(app.cs_access_key, val, sizeof(app.cs_access_key)-1);
        if (!strcmp(key,"sv_host"))    strncpy(app.sv_host,       val, sizeof(app.sv_host)-1);
        if (!strcmp(key,"sv_port"))    app.sv_port           = atoi(val);
        if (!strcmp(key,"sv_name"))    strncpy(app.sv_name,       val, sizeof(app.sv_name)-1);
        if (!strcmp(key,"sv_motd"))    strncpy(app.sv_motd,       val, sizeof(app.sv_motd)-1);
        if (!strcmp(key,"sv_max"))     app.sv_max_clients    = atoi(val);
        if (!strcmp(key,"sv_reqkey"))  app.sv_require_key    = atoi(val)!=0;
        if (!strcmp(key,"sv_access"))  strncpy(app.sv_access_key, val, sizeof(app.sv_access_key)-1);
        if (!strcmp(key,"sv_encrypt")) app.sv_encrypt        = atoi(val);
        if (!strcmp(key,"sv_ekey"))    strncpy(app.sv_encrypt_key, val, sizeof(app.sv_encrypt_key)-1);
    }
    fclose(f);
}

static void sec_header(const char* label)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.60f, 0.15f, 1.0f));
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void draw_connect_window(AppState& app)
{
    ImVec2 avail   = ImGui::GetContentRegionAvail();
    float  avail_h = avail.y;
    float  avail_w = avail.x;
    float  PAD     = 6.0f;
    ImVec2 origin  = ImGui::GetCursorScreenPos();

    ImGui::SetNextWindowPos(origin, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(avail_w, avail_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##connect_outer", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    if (ImGui::BeginTabBar("##connect_tabs")) {

        // ========================================================
        //  TAB: CLIENT
        // ========================================================
        if (ImGui::BeginTabItem("Connect as Client")) {
            ImGui::Spacing();
            if (app.server_running.load()) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f,0.10f,0.06f,1.0f));
                ImGui::BeginChild("##srv_block_banner", ImVec2(-1, 52), false);
                ImGui::SetCursorPos(ImVec2(14.0f, 14.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f,0.55f,0.20f,1.0f));
                ImGui::Text("  Server is running on %s:%d — stop the server first to connect as client.",
                    g_config.host.c_str(), g_config.port);
                ImGui::PopStyleColor();
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }
            float form_w  = 520.0f;
            float log_w   = avail_w - form_w - PAD * 3.0f;
            if (log_w < 200.0f) log_w = 200.0f;
            float inner_h = avail_h - 60.0f;

            // ---- Form ----
            ImGui::BeginChild("##cli_form", ImVec2(form_w, inner_h), true);
            const float LW = 145.0f;

            sec_header("NETWORK");
            ImGui::Text("Host");          ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##host", app.cs_host, sizeof(app.cs_host));
            ImGui::Text("Port");          ImGui::SameLine(LW); ImGui::SetNextItemWidth(130.0f);
            ImGui::InputInt("##port", &app.cs_port, 1, 100);
            app.cs_port = app.cs_port < 1 ? 1 : (app.cs_port > 65535 ? 65535 : app.cs_port);
            ImGui::Text("Timeout (sec)"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(220.0f);
            ImGui::SliderInt("##timeout", &app.cs_timeout_sec, 1, 60);
            ImGui::Text("Auto-reconnect"); ImGui::SameLine(LW);
            ImGui::Checkbox("##autorecon", &app.cs_auto_reconnect);

            sec_header("IDENTITY");
            ImGui::Text("Nickname"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##nick", app.cs_nick, sizeof(app.cs_nick));

            sec_header("ENCRYPTION");
            static const char* cipher_items[] = { "NONE", "XOR", "VIGENERE", "RC4" };
            ImGui::Text("Cipher"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(180.0f);
            ImGui::Combo("##encrypt", &app.cs_encrypt, cipher_items, 4);
            if (app.cs_encrypt != 0) {
                static const char* descs[] = {"","Simple XOR","Vigenere polyalphabetic","RC4 stream"};
                ImGui::SameLine(); ImGui::TextDisabled("— %s", descs[app.cs_encrypt]);
                ImGui::Text("Key"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##key", app.cs_key, sizeof(app.cs_key), ImGuiInputTextFlags_Password);
            } else {
                ImGui::TextDisabled("  No encryption — traffic sent in plaintext.");
            }

            sec_header("SERVER OPTIONS");
            ImGui::Text("Access key"); ImGui::SameLine(LW);
            ImGui::Checkbox("Required##req", &app.cs_require_key);
            if (app.cs_require_key) {
                ImGui::Text("Key"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##access_key", app.cs_access_key,
                                 sizeof(app.cs_access_key), ImGuiInputTextFlags_Password);
            }

            sec_header("PROFILE");
            ImGui::Text("Name"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(180.0f);
            ImGui::InputText("##profname", app.cs_profile_name, sizeof(app.cs_profile_name));
            ImGui::SameLine();
            if (ImGui::Button("Save##psave", ImVec2(70,0))) { save_profile(app); app.log("Profile saved."); }
            ImGui::SameLine();
            if (ImGui::Button("Load##pload", ImVec2(70,0))) { load_profile(app); app.log("Profile loaded."); }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            float btn_w = form_w - ImGui::GetStyle().WindowPadding.x * 2.0f;
            bool already = app.client_connected.load();
            bool srvsup  = app.server_running.load();

            if (!already && !srvsup) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f,0.55f,0.22f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f,0.70f,0.30f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.12f,0.40f,0.16f,1.0f));
                if (ImGui::Button("Connect as Client", ImVec2(btn_w, 44.0f)))
                    app.do_connect();
                ImGui::PopStyleColor(3);
            }
            if (already) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f,0.20f,0.18f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f,0.30f,0.28f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f,0.14f,0.12f,1.0f));
                if (ImGui::Button("Disconnect", ImVec2(btn_w, 36.0f)))
                    app.do_disconnect();
                ImGui::PopStyleColor(3);
            }
            if (srvsup) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.55f,0.15f,1.0f));
                ImGui::TextWrapped("Server is running — stop it first to connect as a client.");
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();

            // ---- Log panel ----
            ImGui::SameLine();
            ImGui::BeginChild("##cli_log", ImVec2(log_w, inner_h), true);
            ImGui::SetWindowFontScale(1.05f); ImGui::Text("Connection Log"); ImGui::SetWindowFontScale(1.0f);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 48.0f);
            if (ImGui::SmallButton(" Clear ")) { std::lock_guard<std::mutex> lk(app.conn_log_mutex); app.conn_log.clear(); }
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f)); ImGui::Separator(); ImGui::PopStyleColor();
            ImGui::BeginChild("##logscroll", ImVec2(0,-1), false, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::lock_guard<std::mutex> lk(app.conn_log_mutex);
                for (size_t i = 0; i < app.conn_log.size(); i++) {
                    const std::string& e = app.conn_log[i];
                    ImVec4 col = ImVec4(0.54f,0.54f,0.56f,1.0f);
                    if (e.find("error")!=std::string::npos || e.find("fail")!=std::string::npos || e.find("FAIL")!=std::string::npos)
                        col = ImVec4(0.88f,0.40f,0.36f,1.0f);
                    else if (e.find("Connect")!=std::string::npos || e.find("connect")!=std::string::npos || e.find("saved")!=std::string::npos || e.find("loaded")!=std::string::npos)
                        col = ImVec4(0.82f,0.58f,0.14f,1.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, col);
                    ImGui::TextWrapped("[%03d] %s", (int)(i+1), e.c_str());
                    ImGui::PopStyleColor();
                }
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // ========================================================
        //  TAB: SERVER
        // ========================================================
        if (ImGui::BeginTabItem("Start Server")) {
            ImGui::Spacing();
            if (app.client_connected.load()) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f,0.10f,0.18f,1.0f));
                ImGui::BeginChild("##cli_block_banner", ImVec2(-1, 52), false);
                ImGui::SetCursorPos(ImVec2(14.0f, 14.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f,0.65f,0.90f,1.0f));
                ImGui::Text("  Client connected to %s:%d — disconnect first to start a server.",
                    g_config.host.c_str(), g_config.port);
                ImGui::PopStyleColor();
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }
            float form_w  = 520.0f;
            float log_w   = avail_w - form_w - PAD * 3.0f;
            if (log_w < 200.0f) log_w = 200.0f;
            float inner_h = avail_h - 60.0f;

            ImGui::BeginChild("##srv_form", ImVec2(form_w, inner_h), true);
            const float LW = 155.0f;

            sec_header("NETWORK");
            ImGui::Text("Bind address"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##svhost", app.sv_host, sizeof(app.sv_host));
            ImGui::TextDisabled("  Use 0.0.0.0 to listen on all interfaces.");
            ImGui::Text("Port"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(130.0f);
            ImGui::InputInt("##svport", &app.sv_port, 1, 100);
            app.sv_port = app.sv_port < 1 ? 1 : (app.sv_port > 65535 ? 65535 : app.sv_port);
            ImGui::Text("Max clients"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(130.0f);
            ImGui::InputInt("##svmaxcli", &app.sv_max_clients, 1, 10);
            app.sv_max_clients = app.sv_max_clients < 1 ? 1 : (app.sv_max_clients > 256 ? 256 : app.sv_max_clients);

            sec_header("IDENTITY");
            ImGui::Text("Server name"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##svname", app.sv_name, sizeof(app.sv_name));
            ImGui::Text("MOTD"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##svmotd", app.sv_motd, sizeof(app.sv_motd));

            sec_header("ENCRYPTION");
            static const char* srv_ciphers[] = { "NONE", "XOR", "VIGENERE", "RC4" };
            ImGui::Text("Cipher"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(180.0f);
            ImGui::Combo("##svencrypt", &app.sv_encrypt, srv_ciphers, 4);
            if (app.sv_encrypt != 0) {
                ImGui::Text("Encrypt Key"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##svekey", app.sv_encrypt_key, sizeof(app.sv_encrypt_key), ImGuiInputTextFlags_Password);
            } else {
                ImGui::TextDisabled("  No encryption — clients connect in plaintext.");
            }

            sec_header("ACCESS CONTROL");
            ImGui::Text("Require key"); ImGui::SameLine(LW);
            ImGui::Checkbox("##svreqkey", &app.sv_require_key);
            if (app.sv_require_key) {
                ImGui::Text("Access key"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputText("##svaccesskey", app.sv_access_key, sizeof(app.sv_access_key), ImGuiInputTextFlags_Password);
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f)); ImGui::Separator(); ImGui::PopStyleColor();
            ImGui::Spacing();

            float btn_w  = form_w - ImGui::GetStyle().WindowPadding.x * 2.0f;
            bool already = app.client_connected.load();
            bool srvsup  = app.server_running.load();

            if (!srvsup && !already) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f,0.36f,0.65f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f,0.48f,0.82f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.12f,0.26f,0.50f,1.0f));
                if (ImGui::Button("Start Server", ImVec2(btn_w, 44.0f)))
                    app.do_start_server();
                ImGui::PopStyleColor(3);
            }
            if (srvsup) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.72f,0.20f,0.18f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f,0.30f,0.28f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f,0.14f,0.12f,1.0f));
                if (ImGui::Button("Stop Server", ImVec2(btn_w, 36.0f)))
                    app.do_stop_server();
                ImGui::PopStyleColor(3);
            }
            if (already) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.55f,0.15f,1.0f));
                ImGui::TextWrapped("Client connected — disconnect first to start a server.");
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();

            // ---- Log ----
            ImGui::SameLine();
            ImGui::BeginChild("##srv_log", ImVec2(log_w, inner_h), true);
            ImGui::SetWindowFontScale(1.05f); ImGui::Text("Server Log"); ImGui::SetWindowFontScale(1.0f);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 48.0f);
            if (ImGui::SmallButton(" Clear ")) { std::lock_guard<std::mutex> lk(app.conn_log_mutex); app.conn_log.clear(); }
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f)); ImGui::Separator(); ImGui::PopStyleColor();
            ImGui::BeginChild("##srvlogscroll", ImVec2(0,-1), false, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::lock_guard<std::mutex> lk(app.conn_log_mutex);
                for (size_t i = 0; i < app.conn_log.size(); i++) {
                    const std::string& e = app.conn_log[i];
                    ImVec4 col = ImVec4(0.54f,0.54f,0.56f,1.0f);
                    if (e.find("error")!=std::string::npos || e.find("fail")!=std::string::npos)
                        col = ImVec4(0.88f,0.40f,0.36f,1.0f);
                    else if (e.find("running")!=std::string::npos || e.find("start")!=std::string::npos || e.find("Start")!=std::string::npos)
                        col = ImVec4(0.55f,0.82f,0.55f,1.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, col);
                    ImGui::TextWrapped("[%03d] %s", (int)(i+1), e.c_str());
                    ImGui::PopStyleColor();
                }
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
