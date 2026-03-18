#include "../include/app.hpp"
#include "../../imgui/imgui.h"
#include <cstring>
#include <cstdio>
#include <string>

static void save_profile(const AppState& app)
{
    FILE* f = fopen("safesocket.profile", "w");
    if (!f) return;
    fprintf(f, "host=%s\n",      app.cs_host);
    fprintf(f, "port=%d\n",      app.cs_port);
    fprintf(f, "nick=%s\n",      app.cs_nick);
    fprintf(f, "encrypt=%d\n",   app.cs_encrypt);
    fprintf(f, "timeout=%d\n",   app.cs_timeout_sec);
    fprintf(f, "autorecon=%d\n", app.cs_auto_reconnect ? 1 : 0);
    fclose(f);
}

static void load_profile(AppState& app)
{
    FILE* f = fopen("safesocket.profile", "r");
    if (!f) return;
    char line[300];
    while (fgets(line, sizeof(line), f)) {
        char key[64]={}, val[256]={};
        if (sscanf(line, "%63[^=]=%255[^\n]", key, val) != 2) continue;
        if (strcmp(key,"host")    ==0){ strncpy(app.cs_host,val,sizeof(app.cs_host)-1); app.cs_host[sizeof(app.cs_host)-1]=0; }
        if (strcmp(key,"nick")    ==0){ strncpy(app.cs_nick,val,sizeof(app.cs_nick)-1); app.cs_nick[sizeof(app.cs_nick)-1]=0; }
        if (strcmp(key,"port")    ==0) app.cs_port           = atoi(val);
        if (strcmp(key,"encrypt") ==0) app.cs_encrypt        = atoi(val);
        if (strcmp(key,"timeout") ==0) app.cs_timeout_sec    = atoi(val);
        if (strcmp(key,"autorecon")==0) app.cs_auto_reconnect = atoi(val)!=0;
    }
    fclose(f);
}

static void section(const char* label)
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
    ImGuiIO& io = ImGui::GetIO();

    // Use the available region inside ##content_area child window
    ImVec2 avail    = ImGui::GetContentRegionAvail();
    float  avail_h  = avail.y;
    float  avail_w  = avail.x;
    float  PAD      = 6.0f;
    float  form_w   = 520.0f;
    float  log_w    = avail_w - form_w - PAD * 3.0f;
    if (log_w < 200.0f) log_w = 200.0f;

    ImVec2 origin = ImGui::GetCursorScreenPos();

    // ── LEFT: form ──────────────────────────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(origin.x + PAD, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(form_w, avail_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##connect_form", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    // Title
    ImGui::SetWindowFontScale(1.12f);
    ImGui::Text("New Connection");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    const float LW = 145.0f;

    section("NETWORK");
    ImGui::Text("Host");          ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##host", app.cs_host, sizeof(app.cs_host));
    ImGui::Text("Port");          ImGui::SameLine(LW); ImGui::SetNextItemWidth(130.0f);
    ImGui::InputInt("##port", &app.cs_port, 1, 100);
    if (app.cs_port < 1) app.cs_port = 1;
    if (app.cs_port > 65535) app.cs_port = 65535;
    ImGui::Text("Timeout (sec)"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderInt("##timeout", &app.cs_timeout_sec, 1, 60);
    ImGui::Text("Auto-reconnect"); ImGui::SameLine(LW);
    ImGui::Checkbox("##autorecon", &app.cs_auto_reconnect);

    section("IDENTITY");
    ImGui::Text("Nickname"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##nick", app.cs_nick, sizeof(app.cs_nick));

    section("ENCRYPTION");
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

    section("SERVER OPTIONS");
    ImGui::Text("Access key"); ImGui::SameLine(LW);
    ImGui::Checkbox("Required##req", &app.cs_require_key);
    if (app.cs_require_key) {
        ImGui::Text("Key"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##access_key", app.cs_access_key,
                         sizeof(app.cs_access_key), ImGuiInputTextFlags_Password);
    }

    section("PROFILE");
    ImGui::Text("Name"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##profname", app.cs_profile_name, sizeof(app.cs_profile_name));
    ImGui::SameLine();
    if (ImGui::Button("Save", ImVec2(70,0))) { save_profile(app); app.log("Profile saved."); }
    ImGui::SameLine();
    if (ImGui::Button("Load", ImVec2(70,0))) { load_profile(app); app.log("Profile loaded."); }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    float total_btn = form_w
        - ImGui::GetStyle().WindowPadding.x * 2.0f
        - ImGui::GetStyle().ItemSpacing.x;
    float btn_w = total_btn * 0.5f;

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.16f,0.16f,0.18f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f,0.52f,0.12f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.58f,0.38f,0.08f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.88f,0.88f,0.86f,1.00f));
    if (ImGui::Button("Connect as Client", ImVec2(btn_w, 44.0f))) {
        app.log("Connecting to " + std::string(app.cs_host) + ":" + std::to_string(app.cs_port) + " ...");
        app.set_status("Not implemented yet — Task 4");
    }
    ImGui::SameLine();
    if (ImGui::Button("Start Server", ImVec2(btn_w, 44.0f))) {
        app.log("Starting server on port " + std::to_string(app.cs_port) + " ...");
        app.set_status("Not implemented yet — Task 4");
    }
    ImGui::PopStyleColor(4);

    ImGui::End();

    // ── RIGHT: log ──────────────────────────────────────────────────────────
    float log_x = origin.x + PAD + form_w + PAD;
    ImGui::SetNextWindowPos(ImVec2(log_x, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(log_w, avail_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##conn_log", NULL,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(1.12f);
    ImGui::Text("Connection Log");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 48.0f);
    if (ImGui::SmallButton(" Clear ")) app.conn_log.clear();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    ImGui::BeginChild("##logscroll", ImVec2(0,-1), false,
        ImGuiWindowFlags_HorizontalScrollbar);
    for (size_t i = 0; i < app.conn_log.size(); i++) {
        const std::string& e = app.conn_log[i];
        ImVec4 col = ImVec4(0.54f,0.54f,0.56f,1.0f);
        if (e.find("error")!=std::string::npos || e.find("fail")!=std::string::npos)
            col = ImVec4(0.88f,0.40f,0.36f,1.0f);
        else if (e.find("saved")!=std::string::npos   ||
                 e.find("loaded")!=std::string::npos  ||
                 e.find("onnect")!=std::string::npos  ||
                 e.find("tarting")!=std::string::npos)
            col = ImVec4(0.82f,0.58f,0.14f,1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("[%03d]  %s", (int)(i+1), e.c_str());
        ImGui::PopStyleColor();
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();

    // Suppress unused warning
    (void)io;
}
