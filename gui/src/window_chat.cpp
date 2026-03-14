/**
 * @file window_chat.cpp
 * @brief Chat panel — message history, peer list, and text input.
 *
 * Used by both client mode (AppScreen::Client) and server mode (AppScreen::Server).
 * In server mode the "Send" button broadcasts to all connected clients.
 * In client mode it broadcasts to all peers.
 */

#include "app.hpp"
#include "imgui.h"

#include <ctime>
#include <cstring>
#include <sstream>
#include <iomanip>

// Format a UNIX timestamp as HH:MM:SS
static std::string format_time(std::time_t t) {
    struct tm* tm_info = localtime(&t);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm_info);
    return buf;
}

void draw_chat_window(AppState& app) {
    bool is_server = (app.screen == AppScreen::Server);

    // ── Layout: peer list on the left, chat on the right ─────────────────────
    float peer_panel_w = 180.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // ── Left: peer / client list ──────────────────────────────────────────────
    ImGui::BeginChild("##peerlist",
        ImVec2(peer_panel_w, avail.y - 2.0f),
        true, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.80f, 1.00f, 1.0f));
    ImGui::Text(is_server ? "Connected Clients" : "Online Peers");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (is_server) {
        auto clients = app.gui_server.client_list();
        for (auto& ci : clients) {
            ImGui::TextDisabled("[%u]", ci.id);
            ImGui::SameLine();
            ImGui::Text("%s", ci.nickname.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("IP: %s", ci.ip.c_str());
            }
        }
        if (clients.empty()) {
            ImGui::TextDisabled("(no clients)");
        }
    } else {
        auto peers = app.gui_client.peers();
        uint32_t my_id = app.gui_client.my_id();
        for (auto& p : peers) {
            bool is_me = (p.id == my_id);
            if (is_me) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));
            ImGui::TextDisabled("[%u]", p.id);
            ImGui::SameLine();
            ImGui::Text("%s%s", p.nickname.c_str(), is_me ? " (you)" : "");
            if (is_me) ImGui::PopStyleColor();
        }
        if (peers.empty()) {
            ImGui::TextDisabled("(connecting...)");
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // ── Right: message history + input ────────────────────────────────────────
    ImGui::BeginChild("##chatright",
        ImVec2(avail.x - peer_panel_w - 8.0f, avail.y - 2.0f),
        false);

    float input_h = 38.0f;
    float history_h = ImGui::GetContentRegionAvail().y - input_h - 8.0f;

    // Message history
    ImGui::BeginChild("##history",
        ImVec2(0, history_h),
        true,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& msg : app.messages) {
        // Timestamp
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.50f, 1.0f));
        ImGui::Text("[%s]", format_time(msg.timestamp).c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Sender
        if (msg.is_system) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.40f, 1.0f));
            ImGui::Text("* %s", msg.text.c_str());
            ImGui::PopStyleColor();
        } else if (msg.is_private) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.75f, 0.30f, 1.0f));
            ImGui::Text("[PM] %s:", msg.sender.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.text.c_str());
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.75f, 1.00f, 1.0f));
            ImGui::Text("%s:", msg.sender.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextWrapped("%s", msg.text.c_str());
        }
    }

    // Auto-scroll to bottom when new messages arrive
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    // Input row
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);

    bool enter_pressed = ImGui::InputText(
        "##chatinput", app.chat_input, sizeof(app.chat_input),
        ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine();
    bool send_clicked = ImGui::Button("Send", ImVec2(70.0f, 0.0f));

    if ((enter_pressed || send_clicked) && app.chat_input[0] != '\0') {
        std::string text = app.chat_input;
        app.chat_input[0] = '\0';

        if (is_server) {
            app.gui_server.broadcast(text);
            app.push_message("Server", text, false, false);
        } else {
            app.gui_client.send_broadcast(text);
            // The echo will arrive via the receive thread
        }

        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::EndChild(); // chatright
}
