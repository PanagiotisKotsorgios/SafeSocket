/**
 * @file window_connect.cpp
 * @brief Connect / login screen drawn when AppState::screen == Connect.
 */

#include "app.hpp"
#include "imgui.h"
#include "../../src/network.hpp"
#include "../../src/crypto.hpp"

#include <cstring>

static const char* ENCRYPT_NAMES[] = { "NONE", "XOR", "VIGENERE", "RC4" };
static const int   ENCRYPT_COUNT   = 4;

void draw_connect_window(AppState& app) {
    ImGuiIO& io = ImGui::GetIO();

    // Centre the connect panel
    ImVec2 panel_size(480.0f, 440.0f);
    ImVec2 pos(
        (io.DisplaySize.x - panel_size.x) * 0.5f,
        (io.DisplaySize.y - panel_size.y) * 0.5f
    );
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(panel_size);
    ImGui::SetNextWindowBgAlpha(0.95f);

    ImGui::Begin("##connect",
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.70f, 1.00f, 1.0f));
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("SafeSocket v0.0.2");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Connection fields ─────────────────────────────────────────────────────
    ImGui::Text("Host");
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##host", app.cs_host, sizeof(app.cs_host));

    ImGui::Text("Port");
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputInt("##port", &app.cs_port, 0, 0);
    if (app.cs_port < 1)    app.cs_port = 1;
    if (app.cs_port > 65535) app.cs_port = 65535;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Identity ──────────────────────────────────────────────────────────────
    ImGui::Text("Nickname");
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##nick", app.cs_nick, sizeof(app.cs_nick));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Encryption ────────────────────────────────────────────────────────────
    ImGui::Text("Encryption");
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##enc", &app.cs_encrypt, ENCRYPT_NAMES, ENCRYPT_COUNT);

    if (app.cs_encrypt != 0) {
        ImGui::Text("Key");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##enckey", app.cs_key, sizeof(app.cs_key),
                         ImGuiInputTextFlags_Password);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Access key (server mode) ──────────────────────────────────────────────
    ImGui::Checkbox("Require access key (server only)", &app.cs_require_key);
    if (app.cs_require_key) {
        ImGui::Text("Access key");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##akey", app.cs_access_key, sizeof(app.cs_access_key),
                         ImGuiInputTextFlags_Password);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Buttons ───────────────────────────────────────────────────────────────
    float btn_w = (panel_size.x - 40.0f) * 0.5f;

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.42f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.56f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.12f, 0.30f, 0.12f, 1.0f));

    if (ImGui::Button("Connect as Client", ImVec2(btn_w, 38.0f))) {
        // Apply settings to g_config
        g_config.host        = app.cs_host;
        g_config.port        = app.cs_port;
        g_config.nickname    = app.cs_nick;
        g_config.encrypt_type = static_cast<EncryptType>(app.cs_encrypt);
        g_config.encrypt_key  = (app.cs_encrypt != 0) ? app.cs_key : "";

        if (!net_init()) {
            app.set_status("Failed to initialise networking.", true);
        } else if (app.gui_client.connect(app.cs_host, app.cs_port)) {
            app.screen = AppScreen::Client;
            app.set_status("Connected to " + std::string(app.cs_host) + ":" +
                           std::to_string(app.cs_port));
        } else {
            app.set_status("Connection failed. Is the server running?", true);
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, 12);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.42f, 0.18f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.56f, 0.24f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f, 0.12f, 0.12f, 1.0f));

    if (ImGui::Button("Start Server", ImVec2(btn_w, 38.0f))) {
        g_config.host         = app.cs_host;
        g_config.port         = app.cs_port;
        g_config.encrypt_type = static_cast<EncryptType>(app.cs_encrypt);
        g_config.encrypt_key  = (app.cs_encrypt != 0) ? app.cs_key : "";
        g_config.require_key  = app.cs_require_key;
        g_config.access_key   = app.cs_require_key ? app.cs_access_key : "";

        if (!net_init()) {
            app.set_status("Failed to initialise networking.", true);
        } else if (app.gui_server.start(app.cs_host, app.cs_port)) {
            app.screen = AppScreen::Server;
            app.set_status("Server started on port " + std::to_string(app.cs_port));
        } else {
            app.set_status("Server failed to start. Port in use?", true);
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
}
