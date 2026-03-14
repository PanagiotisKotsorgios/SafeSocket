/**
 * @file window_files.cpp
 * @brief File transfer queue panel — progress bars, speeds, cancel buttons.
 *
 * NOTE: Progress bars reflect chunk count in v0.0.2.
 * Byte-accurate progress via MSG_FILE_PROGRESS arrives in v0.0.4.
 */

#include "app.hpp"
#include "imgui.h"

#include <cstring>
#include <sstream>
#include <iomanip>

static std::string fmt_bytes(uint64_t bytes) {
    char buf[32];
    if (bytes >= 1024ULL * 1024 * 1024)
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    else if (bytes >= 1024 * 1024)
        snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024)
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    else
        snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    return buf;
}

void draw_files_window(AppState& app) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.80f, 1.00f, 1.0f));
    ImGui::Text("File Transfers");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    if (app.transfers.empty()) {
        ImGui::TextDisabled("No active or recent transfers.");
        ImGui::Spacing();
        ImGui::TextDisabled("Use /sendfile in the chat or right-click a peer to send.");
        return;
    }

    for (size_t i = 0; i < app.transfers.size(); ++i) {
        auto& t = app.transfers[i];

        ImGui::PushID((int)i);

        // Direction indicator
        ImGui::PushStyleColor(ImGuiCol_Text,
            t.is_upload ? ImVec4(0.4f, 0.9f, 0.4f, 1.0f)
                        : ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        ImGui::Text(t.is_upload ? " ↑" : " ↓");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Filename and peer
        ImGui::Text("%s", t.filename.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", t.peer.c_str());

        // Progress bar
        float frac = (t.total_bytes > 0)
            ? (float)t.bytes_done / (float)t.total_bytes
            : 0.0f;

        char overlay[64];
        if (t.rejected)
            snprintf(overlay, sizeof(overlay), "Rejected");
        else if (t.finished)
            snprintf(overlay, sizeof(overlay), "Done — %s",
                     fmt_bytes(t.total_bytes).c_str());
        else
            snprintf(overlay, sizeof(overlay), "%s / %s",
                     fmt_bytes(t.bytes_done).c_str(),
                     fmt_bytes(t.total_bytes).c_str());

        if (t.rejected) {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        } else if (t.finished) {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        }

        ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f), overlay);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::PopID();
    }
}
