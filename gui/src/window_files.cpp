#include "../include/app.hpp"
#include "imgui.h"
#include <cstring>
#include <string>
#include <mutex>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#endif

static void open_folder(const char* path)
{
#ifdef _WIN32
    ShellExecuteA(NULL, "explore", path, NULL, NULL, SW_SHOWDEFAULT);
#else
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", path);
    system(cmd);
#endif
}

void draw_files_window(AppState& app)
{
    ImVec2 avail  = ImGui::GetContentRegionAvail();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float total_h = avail.y;
    float total_w = avail.x;

    // Top half: send file panel; bottom half: transfers list
    float send_h     = 200.0f;
    float list_h     = total_h - send_h - 20.0f;

    // ============================================================
    // SEND FILE PANEL
    // ============================================================
    ImGui::SetNextWindowPos(ImVec2(origin.x, origin.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(total_w, send_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##files_send", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(1.05f);
    ImGui::Text("Send File");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // File path input
    const float LW = 120.0f;
    ImGui::Text("File path");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(total_w - LW - 80.0f
        - ImGui::GetStyle().WindowPadding.x * 2.0f
        - ImGui::GetStyle().ItemSpacing.x * 2.0f);
    ImGui::InputText("##filepath", app.sf_filepath, sizeof(app.sf_filepath));
    ImGui::SameLine();
    if (ImGui::Button("Browse##filebrowse", ImVec2(70, 0))) {
#ifdef _WIN32
        // Simple Windows file open dialog
        OPENFILENAMEA ofn;
        char szFile[512] = {};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile   = szFile;
        ofn.nMaxFile    = sizeof(szFile);
        ofn.lpstrFilter = "All Files\0*.*\0";
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            strncpy(app.sf_filepath, szFile, sizeof(app.sf_filepath)-1);
        }
#else
        // No native dialog on Linux in this build
        app.log("Browse not available – type path manually.");
#endif
    }

    // Destination selector
    ImGui::Text("Destination");
    ImGui::SameLine(LW);
    ImGui::RadioButton("Server##tosrv",  &app.sf_target_id, 0);
    ImGui::SameLine();

    {
        std::lock_guard<std::mutex> lk(app.peers_mutex);
        if (app.peers.empty()) {
            ImGui::BeginDisabled();
        }
        ImGui::RadioButton("Client##tocli", &app.sf_target_id, 1);
        if (app.peers.empty()) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("(no peers)");
        }
    }

    // Client ID combo (only shown when destination = client)
    static int send_to_client_id = 0;
    if (app.sf_target_id != 0) {
        ImGui::Text("Client ID");
        ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(160.0f);
        // Build combo from peers
        std::string preview = std::to_string(send_to_client_id);
        if (ImGui::BeginCombo("##sendtoclientid", preview.c_str())) {
            std::lock_guard<std::mutex> lk(app.peers_mutex);
            for (auto& p : app.peers) {
                if (p.id == app.my_id) continue;
                std::string label = "[" + std::to_string(p.id) + "] " + p.nickname;
                bool sel = (send_to_client_id == (int)p.id);
                if (ImGui::Selectable(label.c_str(), sel))
                    send_to_client_id = (int)p.id;
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();

    bool connected = app.client_connected.load();
    if (!connected) ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f,0.52f,0.12f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f,0.68f,0.20f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.58f,0.38f,0.08f,1.0f));
    if (ImGui::Button("Send File", ImVec2(120.0f, 32.0f))) {
        std::string path = app.sf_filepath;
        if (!path.empty()) {
            bool to_srv = (app.sf_target_id == 0);
            uint32_t tid = to_srv ? 0 : (uint32_t)send_to_client_id;
            app.send_file_gui(path, to_srv, tid);
        } else {
            app.set_status("No file path specified.", true);
        }
    }
    ImGui::PopStyleColor(3);

    if (!connected) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f,0.40f,0.36f,1.0f));
        ImGui::Text("Not connected");
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (ImGui::Button("Open Downloads", ImVec2(140, 32.0f))) {
        open_folder(g_config.download_dir.c_str());
    }

    ImGui::End();

    // ============================================================
    // TRANSFERS LIST
    // ============================================================
    float list_y = origin.y + send_h + 8.0f;
    ImGui::SetNextWindowPos(ImVec2(origin.x, list_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(total_w, list_h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##files_list", NULL,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(1.05f);
    ImGui::Text("Transfer History");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
    if (ImGui::SmallButton("Clear All")) {
        std::lock_guard<std::mutex> lk(app.transfers_mutex);
        app.transfers.clear();
    }
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.20f,0.20f,0.22f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    // Column headers
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.60f, 0.15f, 1.0f));
    ImGui::Text("%-4s  %-30s  %-20s  %-10s  %s",
        "Dir", "Filename", "Peer", "Status", "Progress");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.22f,0.22f,0.24f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    ImGui::BeginChild("##xferscroll", ImVec2(0,-1), false);

    {
        std::lock_guard<std::mutex> lk(app.transfers_mutex);
        if (app.transfers.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.45f,0.47f,1.0f));
            ImGui::Text("  No transfers yet.");
            ImGui::PopStyleColor();
        }
        for (size_t i = 0; i < app.transfers.size(); ++i) {
            auto& tr = app.transfers[i];
            ImGui::PushID((int)i);

            const char* dir = tr.is_upload ? " UP " : "DOWN";
            ImVec4 dir_col = tr.is_upload
                ? ImVec4(0.55f, 0.80f, 0.55f, 1.0f)
                : ImVec4(0.55f, 0.65f, 0.95f, 1.0f);

            // Dir
            ImGui::PushStyleColor(ImGuiCol_Text, dir_col);
            ImGui::Text("%s", dir);
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // Filename (truncated)
            ImGui::Text("%-30s", tr.filename.c_str());
            ImGui::SameLine();

            // Peer
            ImGui::Text("%-20s", tr.peer.c_str());
            ImGui::SameLine();

            // Status
            const char* status;
            ImVec4 st_col;
            if (tr.failed) {
                status = "FAILED";
                st_col = ImVec4(0.88f, 0.35f, 0.30f, 1.0f);
            } else if (tr.done) {
                status = "Done";
                st_col = ImVec4(0.55f, 0.85f, 0.55f, 1.0f);
            } else {
                status = "...";
                st_col = ImVec4(0.85f, 0.75f, 0.30f, 1.0f);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, st_col);
            ImGui::Text("%-10s", status);
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // Progress bar
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                tr.failed ? ImVec4(0.72f,0.20f,0.18f,1.0f)
                          : ImVec4(0.75f,0.52f,0.12f,1.0f));
            ImGui::ProgressBar(tr.progress, ImVec2(120.0f, 0.0f));
            ImGui::PopStyleColor();

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
    ImGui::End();
}
