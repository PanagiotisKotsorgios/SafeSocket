#include "../include/app.hpp"
#include "imgui.h"
#include <cstring>
#include <string>

static void setting_section(const char* label)
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

void draw_settings_window(AppState& app)
{
    ImVec2 avail  = ImGui::GetContentRegionAvail();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    ImGui::SetNextWindowPos(origin, ImGuiCond_Always);
    ImGui::SetNextWindowSize(avail, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##settings_win", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(1.08f);
    ImGui::Text("Settings");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    ImGui::BeginChild("##settings_scroll", ImVec2(0, -50.0f), false);

    const float LW = 175.0f;

    // ---- Theme ----
    setting_section("APPEARANCE");
    ImGui::Text("Theme");
    ImGui::SameLine(LW);
    if (ImGui::RadioButton("Dark", app.dark_theme)) {
        app.dark_theme = true;
        apply_dark_theme();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Light", !app.dark_theme)) {
        app.dark_theme = false;
        apply_light_theme();
    }

    // ---- File Transfer ----
    setting_section("FILE TRANSFER");

    ImGui::Text("Download directory");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##dldir", app.set_download_dir, sizeof(app.set_download_dir));

    ImGui::Text("Auto-accept files");
    ImGui::SameLine(LW);
    ImGui::Checkbox("##autoaccept", &app.set_auto_accept);
    ImGui::SameLine();
    ImGui::TextDisabled("(accept without prompt)");

    ImGui::Text("Max file size (MB)");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputInt("##maxfile", &app.set_max_file_mb, 1, 10);
    if (app.set_max_file_mb < 0) app.set_max_file_mb = 0;
    ImGui::SameLine();
    ImGui::TextDisabled("(0 = unlimited)");

    // ---- Logging ----
    setting_section("LOGGING");

    ImGui::Text("Log to file");
    ImGui::SameLine(LW);
    ImGui::Checkbox("##logtf", &app.set_log_to_file);

    if (app.set_log_to_file) {
        ImGui::Text("Log file path");
        ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##logfile", app.set_log_file, sizeof(app.set_log_file));
    }

    ImGui::Text("Verbose output");
    ImGui::SameLine(LW);
    ImGui::Checkbox("##verbose", &app.set_verbose);
    ImGui::SameLine();
    ImGui::TextDisabled("(extra diagnostic messages)");

    // ---- Keepalive ----
    setting_section("KEEPALIVE");

    ImGui::Text("Enable keepalive");
    ImGui::SameLine(LW);
    ImGui::Checkbox("##keepalive", &app.set_keepalive);

    if (app.set_keepalive) {
        ImGui::Text("Ping interval (sec)");
        ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderInt("##pingint", &app.set_ping_interval, 5, 120);
    }

    // ---- Config file ----
    setting_section("CONFIGURATION FILE");

    if (ImGui::Button("Save to safesocket.conf", ImVec2(220, 28))) {
        app.apply_settings();
        g_config.save("safesocket.conf");
        app.log("Config saved to safesocket.conf");
        app.set_status("Config saved.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load from safesocket.conf", ImVec2(225, 28))) {
        app.load_settings_from_config();
        app.log("Config loaded from safesocket.conf");
        app.set_status("Config loaded.");
    }

    ImGui::Spacing();

    ImGui::EndChild();

    // ---- Apply button ----
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f,0.52f,0.12f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f,0.68f,0.20f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.58f,0.38f,0.08f,1.0f));
    if (ImGui::Button("Apply Settings", ImVec2(160, 32))) {
        app.apply_settings();
        app.set_status("Settings applied.");
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
}
