/**
 * @file window_server.cpp
 * @brief Server control panel — client table, kick, private message, file send.
 */

#include "app.hpp"
#include "imgui.h"

#include <cstring>
#include <ctime>
#include <sstream>

static std::string uptime_str(std::time_t connected_at) {
    long secs = (long)(std::time(nullptr) - connected_at);
    if (secs < 0) secs = 0;
    char buf[32];
    snprintf(buf, sizeof(buf), "%ldh %02ldm %02lds",
             secs / 3600, (secs % 3600) / 60, secs % 60);
    return buf;
}

void draw_server_window(AppState& app) {
    auto clients = app.gui_server.client_list();

    // ── Stats bar ─────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.80f, 1.00f, 1.0f));
    ImGui::Text("Connected: %zu / %d",
        clients.size(), g_config.max_clients);
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 30);
    if (ImGui::Button("Stop Server")) {
        app.gui_server.stop();
        app.screen = AppScreen::Connect;
        app.messages.clear();
        app.set_status("Server stopped.");
    }
    ImGui::Separator();
    ImGui::Spacing();

    // ── Broadcast input ───────────────────────────────────────────────────────
    static char bcast_buf[1024] = {};
    ImGui::Text("Broadcast:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.0f);
    bool enter = ImGui::InputText("##bcast", bcast_buf, sizeof(bcast_buf),
                                  ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    bool send_btn = ImGui::Button("Send##bcast", ImVec2(80.0f, 0.0f));
    if ((enter || send_btn) && bcast_buf[0] != '\0') {
        app.gui_server.broadcast(bcast_buf);
        app.push_message("Server", bcast_buf, false, false);
        bcast_buf[0] = '\0';
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Client table ──────────────────────────────────────────────────────────
    static int  selected_id   = -1;
    static char pm_buf[512]   = {};
    static char file_buf[512] = {};
    static char kick_reason[256] = {};

    if (ImGui::BeginTable("##clients",
        5,
        ImGuiTableFlags_Borders       |
        ImGuiTableFlags_RowBg         |
        ImGuiTableFlags_ScrollY       |
        ImGuiTableFlags_Resizable,
        ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 140.0f)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID",       ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Nickname", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("IP",       ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Uptime",   ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Action",   ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (auto& ci : clients) {
            ImGui::TableNextRow();
            bool row_selected = (selected_id == (int)ci.id);

            ImGui::TableSetColumnIndex(0);
            char id_label[16];
            snprintf(id_label, sizeof(id_label), "%u", ci.id);
            if (ImGui::Selectable(id_label, row_selected,
                ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0)))
            {
                selected_id = (int)ci.id;
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", ci.nickname.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("%s", ci.ip.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextDisabled("%s", uptime_str(ci.connected_at).c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::PushID((int)ci.id);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
            if (ImGui::SmallButton("Kick")) {
                app.gui_server.kick(ci.id, "Kicked by server operator");
                app.push_message("Server",
                    "Kicked client [" + std::to_string(ci.id) + "] " + ci.nickname,
                    false, true);
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // ── Actions for selected client ───────────────────────────────────────────
    ImGui::Spacing();
    if (selected_id >= 0) {
        ImGui::Text("Selected: [%d]", selected_id);
        ImGui::SameLine(0, 14);

        // Private message
        ImGui::SetNextItemWidth(260.0f);
        bool pm_enter = ImGui::InputText("##pm", pm_buf, sizeof(pm_buf),
                                         ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        bool pm_btn = ImGui::Button("PM", ImVec2(40.0f, 0.0f));
        if ((pm_enter || pm_btn) && pm_buf[0] != '\0') {
            app.gui_server.private_msg((uint32_t)selected_id, pm_buf);
            app.push_message("You -> [" + std::to_string(selected_id) + "]",
                             pm_buf, true, false);
            pm_buf[0] = '\0';
        }

        ImGui::SameLine(0, 10);

        // Send file
        ImGui::SetNextItemWidth(220.0f);
        bool file_enter = ImGui::InputText("##fpath", file_buf, sizeof(file_buf),
                                           ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        bool file_btn = ImGui::Button("Send File", ImVec2(80.0f, 0.0f));
        if ((file_enter || file_btn) && file_buf[0] != '\0') {
            app.gui_server.send_file_to_client((uint32_t)selected_id, file_buf);
            file_buf[0] = '\0';
        }
    } else {
        ImGui::TextDisabled("Select a client row to send a private message or file.");
    }
}
