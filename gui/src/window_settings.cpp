/**
 * @file window_settings.cpp
 * @brief Settings panel — live editor for all g_config fields.
 *
 * Changes take effect immediately (same semantics as the CLI /set command).
 * Save / Load buttons write to and read from safesocket.conf.
 */

#include "app.hpp"
#include "imgui.h"
#include "../../src/config.hpp"
#include "../../src/crypto.hpp"

#include <cstring>
#include <string>

static const char* ENCRYPT_NAMES[] = { "NONE", "XOR", "VIGENERE", "RC4" };

static int encrypt_type_to_idx(EncryptType t) {
    switch (t) {
    case EncryptType::XOR:      return 1;
    case EncryptType::VIGENERE: return 2;
    case EncryptType::RC4:      return 3;
    default:                    return 0;
    }
}

void draw_settings_window(AppState& app) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.80f, 1.00f, 1.0f));
    ImGui::Text("Settings  (changes apply immediately)");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    bool changed = false;

    // ── Network ───────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.40f, 1.0f));
    ImGui::Text("Network");
    ImGui::PopStyleColor();

    static char s_host[128];
    static bool s_init = false;
    if (!s_init) {
        strncpy(s_host, g_config.host.c_str(), sizeof(s_host) - 1);
        s_init = true;
    }

    if (ImGui::InputText("Host##s", s_host, sizeof(s_host),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.host = s_host;
        changed = true;
    }

    int port = g_config.port;
    if (ImGui::InputInt("Port##s", &port)) {
        if (port >= 1 && port <= 65535) { g_config.port = port; changed = true; }
    }

    int max_cl = g_config.max_clients;
    if (ImGui::InputInt("Max clients##s", &max_cl)) {
        if (max_cl >= 1 && max_cl <= 256) { g_config.max_clients = max_cl; changed = true; }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Identity ──────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.40f, 1.0f));
    ImGui::Text("Identity");
    ImGui::PopStyleColor();

    static char s_nick[64];
    strncpy(s_nick, g_config.nickname.c_str(), sizeof(s_nick) - 1);
    if (ImGui::InputText("Nickname##s", s_nick, sizeof(s_nick),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.nickname = s_nick; changed = true;
    }

    static char s_srv_name[128];
    strncpy(s_srv_name, g_config.server_name.c_str(), sizeof(s_srv_name) - 1);
    if (ImGui::InputText("Server name##s", s_srv_name, sizeof(s_srv_name),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.server_name = s_srv_name; changed = true;
    }

    static char s_motd[256];
    strncpy(s_motd, g_config.motd.c_str(), sizeof(s_motd) - 1);
    if (ImGui::InputText("MOTD##s", s_motd, sizeof(s_motd),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.motd = s_motd; changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Encryption ────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.40f, 1.0f));
    ImGui::Text("Encryption");
    ImGui::PopStyleColor();

    int enc_idx = encrypt_type_to_idx(g_config.encrypt_type);
    if (ImGui::Combo("Cipher##s", &enc_idx, ENCRYPT_NAMES, 4)) {
        g_config.encrypt_type = static_cast<EncryptType>(enc_idx);
        changed = true;
    }

    static char s_enckey[256];
    strncpy(s_enckey, g_config.encrypt_key.c_str(), sizeof(s_enckey) - 1);
    if (ImGui::InputText("Key##s", s_enckey, sizeof(s_enckey),
                         ImGuiInputTextFlags_Password |
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.encrypt_key = s_enckey; changed = true;
    }

    bool rk = g_config.require_key;
    if (ImGui::Checkbox("Require access key##s", &rk)) {
        g_config.require_key = rk; changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── File transfer ─────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.40f, 1.0f));
    ImGui::Text("File Transfer");
    ImGui::PopStyleColor();

    static char s_dldir[256];
    strncpy(s_dldir, g_config.download_dir.c_str(), sizeof(s_dldir) - 1);
    if (ImGui::InputText("Download dir##s", s_dldir, sizeof(s_dldir),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.download_dir = s_dldir; changed = true;
    }

    bool aa = g_config.auto_accept_files;
    if (ImGui::Checkbox("Auto-accept files##s", &aa)) {
        g_config.auto_accept_files = aa; changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Logging ───────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.40f, 1.0f));
    ImGui::Text("Logging");
    ImGui::PopStyleColor();

    bool ltf = g_config.log_to_file;
    if (ImGui::Checkbox("Log to file##s", &ltf)) {
        g_config.log_to_file = ltf; changed = true;
    }

    static char s_lf[256];
    strncpy(s_lf, g_config.log_file.c_str(), sizeof(s_lf) - 1);
    if (ImGui::InputText("Log file##s", s_lf, sizeof(s_lf),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        g_config.log_file = s_lf; changed = true;
    }

    bool vb = g_config.verbose;
    if (ImGui::Checkbox("Verbose##s", &vb)) { g_config.verbose = vb; changed = true; }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Save / Load ───────────────────────────────────────────────────────────
    if (ImGui::Button("Save to safesocket.conf", ImVec2(220.0f, 0.0f))) {
        if (g_config.save("safesocket.conf"))
            app.set_status("Config saved to safesocket.conf");
        else
            app.set_status("Failed to save config.", true);
    }

    ImGui::SameLine(0, 12);

    if (ImGui::Button("Load safesocket.conf", ImVec2(200.0f, 0.0f))) {
        if (g_config.load("safesocket.conf"))
            app.set_status("Config loaded from safesocket.conf");
        else
            app.set_status("Could not load safesocket.conf.", true);
    }

    if (changed) app.settings_changed = true;
}
