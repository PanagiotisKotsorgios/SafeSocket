/**
 * @file main_gui.cpp
 * SafeSocket GUI — themes, full navbar, modals, entry point.
 */
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "../include/app.hpp"
#include "../include/security.hpp"
#include <cstring>
#include <string>
#include <cstdio>
#include <ctime>
#include <mutex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#endif

void apply_dark_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    c[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    c[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    c[ImGuiCol_TitleBg]              = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_MenuBarBg]            = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.36f, 0.36f, 0.38f, 1.00f);
    c[ImGuiCol_CheckMark]            = ImVec4(0.85f, 0.60f, 0.15f, 1.00f);
    c[ImGuiCol_SliderGrab]           = ImVec4(0.75f, 0.52f, 0.12f, 1.00f);
    c[ImGuiCol_SliderGrabActive]     = ImVec4(0.92f, 0.68f, 0.20f, 1.00f);
    c[ImGuiCol_Button]               = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.75f, 0.52f, 0.12f, 1.00f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.58f, 0.38f, 0.08f, 1.00f);
    c[ImGuiCol_Header]               = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.75f, 0.52f, 0.12f, 0.80f);
    c[ImGuiCol_Separator]            = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_SeparatorHovered]     = ImVec4(0.75f, 0.52f, 0.12f, 0.80f);
    c[ImGuiCol_SeparatorActive]      = ImVec4(0.85f, 0.60f, 0.15f, 1.00f);
    c[ImGuiCol_ResizeGrip]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_ResizeGripActive]     = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_Tab]                  = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_TabHovered]           = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_TabActive]            = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    c[ImGuiCol_TabUnfocused]         = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    c[ImGuiCol_Text]                 = ImVec4(0.88f, 0.88f, 0.86f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.42f, 0.42f, 0.44f, 1.00f);
    c[ImGuiCol_TextSelectedBg]       = ImVec4(0.75f, 0.52f, 0.12f, 0.30f);
    c[ImGuiCol_NavHighlight]         = ImVec4(0.75f, 0.52f, 0.12f, 1.00f);
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);

    style.WindowRounding          = 0.0f;
    style.ChildRounding           = 0.0f;
    style.FrameRounding           = 0.0f;
    style.PopupRounding           = 0.0f;
    style.ScrollbarRounding       = 0.0f;
    style.GrabRounding            = 0.0f;
    style.TabRounding             = 0.0f;
    style.WindowBorderSize        = 1.0f;
    style.ChildBorderSize         = 1.0f;
    style.FrameBorderSize         = 0.0f;
    style.PopupBorderSize         = 1.0f;
    style.FramePadding            = ImVec2(10.0f, 5.0f);
    style.ItemSpacing             = ImVec2(10.0f, 6.0f);
    style.ItemInnerSpacing        = ImVec2(6.0f,  4.0f);
    style.WindowPadding           = ImVec2(14.0f, 12.0f);
    style.IndentSpacing           = 18.0f;
    style.ScrollbarSize           = 10.0f;
    style.GrabMinSize             = 8.0f;
}

void apply_light_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // Backgrounds — light grey layers, clearly distinct
    c[ImGuiCol_WindowBg]             = ImVec4(0.89f, 0.89f, 0.88f, 1.00f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.84f, 0.84f, 0.83f, 1.00f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.93f, 0.93f, 0.92f, 1.00f);

    // Borders — dark enough to be clearly visible
    c[ImGuiCol_Border]               = ImVec4(0.38f, 0.36f, 0.34f, 1.00f);
    c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Input fields — white so they pop against the grey bg
    c[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.94f, 0.94f, 0.92f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.88f, 0.88f, 0.86f, 1.00f);

    // Title / menu — slightly darker than window
    c[ImGuiCol_TitleBg]              = ImVec4(0.78f, 0.77f, 0.75f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.74f, 0.73f, 0.71f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.78f, 0.77f, 0.75f, 1.00f);
    c[ImGuiCol_MenuBarBg]            = ImVec4(0.74f, 0.73f, 0.71f, 1.00f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.82f, 0.81f, 0.80f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.48f, 0.47f, 0.45f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.35f, 0.33f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.24f, 0.23f, 0.22f, 1.00f);

    // Accent — amber, same as dark
    c[ImGuiCol_CheckMark]            = ImVec4(0.62f, 0.40f, 0.04f, 1.00f);
    c[ImGuiCol_SliderGrab]           = ImVec4(0.60f, 0.38f, 0.04f, 1.00f);
    c[ImGuiCol_SliderGrabActive]     = ImVec4(0.74f, 0.50f, 0.08f, 1.00f);

    // Buttons — dark so they are clearly clickable on light bg
    c[ImGuiCol_Button]               = ImVec4(0.22f, 0.22f, 0.21f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.62f, 0.40f, 0.04f, 1.00f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.46f, 0.28f, 0.02f, 1.00f);

    // Header / selectable
    c[ImGuiCol_Header]               = ImVec4(0.80f, 0.79f, 0.77f, 1.00f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.70f, 0.69f, 0.67f, 1.00f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.62f, 0.40f, 0.04f, 0.80f);

    // Separators — clearly visible dark line
    c[ImGuiCol_Separator]            = ImVec4(0.36f, 0.35f, 0.33f, 1.00f);
    c[ImGuiCol_SeparatorHovered]     = ImVec4(0.62f, 0.40f, 0.04f, 1.00f);
    c[ImGuiCol_SeparatorActive]      = ImVec4(0.74f, 0.50f, 0.08f, 1.00f);

    // Resize grip — hidden
    c[ImGuiCol_ResizeGrip]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_ResizeGripActive]     = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Tabs
    c[ImGuiCol_Tab]                  = ImVec4(0.74f, 0.73f, 0.71f, 1.00f);
    c[ImGuiCol_TabHovered]           = ImVec4(0.64f, 0.63f, 0.61f, 1.00f);
    c[ImGuiCol_TabActive]            = ImVec4(0.89f, 0.89f, 0.88f, 1.00f);
    c[ImGuiCol_TabUnfocused]         = ImVec4(0.74f, 0.73f, 0.71f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.82f, 0.81f, 0.80f, 1.00f);

    // TEXT — pure near-black, maximum contrast
    c[ImGuiCol_Text]                 = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.38f, 0.37f, 0.35f, 1.00f);
    c[ImGuiCol_TextSelectedBg]       = ImVec4(0.62f, 0.40f, 0.04f, 0.40f);

    // Nav
    c[ImGuiCol_NavHighlight]         = ImVec4(0.62f, 0.40f, 0.04f, 1.00f);
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.45f);

    // Sharp geometry — identical to dark
    style.WindowRounding          = 0.0f;
    style.ChildRounding           = 0.0f;
    style.FrameRounding           = 0.0f;
    style.PopupRounding           = 0.0f;
    style.ScrollbarRounding       = 0.0f;
    style.GrabRounding            = 0.0f;
    style.TabRounding             = 0.0f;
    style.WindowBorderSize        = 1.0f;
    style.ChildBorderSize         = 1.0f;
    style.FrameBorderSize         = 1.0f;
    style.PopupBorderSize         = 1.0f;
    style.FramePadding            = ImVec2(10.0f, 5.0f);
    style.ItemSpacing             = ImVec2(10.0f, 6.0f);
    style.ItemInnerSpacing        = ImVec2(6.0f,  4.0f);
    style.WindowPadding           = ImVec2(14.0f, 12.0f);
    style.IndentSpacing           = 18.0f;
    style.ScrollbarSize           = 10.0f;
    style.GrabMinSize             = 8.0f;
}


// ============================================================
//  Navbar-only modal/preference state (not persisted)
// ============================================================
static struct NavState {
    bool show_db_config   = false;
    bool show_export      = false;
    bool show_hotkeys     = false;
    bool show_net_diag    = false;
    bool show_log_viewer  = false;
    bool show_prefs       = false;

    // DB
    int  db_type          = 0;   // 0=None 1=SQLite 2=MySQL 3=PostgreSQL 4=Redis
    char db_host[128]     = "127.0.0.1";
    int  db_port          = 3306;
    char db_name[128]     = "safesocket";
    char db_user[128]     = "root";
    char db_pass[256]     = {};
    bool db_ssl           = false;
    int  db_pool_size     = 4;
    int  db_timeout_sec   = 10;
    char db_sqlite_path[512] = "safesocket.db";
    bool db_connected     = false;
    char db_status[256]   = "Not connected";

    // Export
    int  export_fmt       = 0;
    char export_path[512] = "export_safesocket";
    bool export_messages  = true;
    bool export_transfers = true;
    bool export_log       = true;
    bool export_config    = false;

    // Prefs
    float font_scale      = 1.0f;
    bool  show_timestamps = true;
    bool  compact_mode    = false;
    bool  show_status_bar = true;
    bool  show_footer     = true;
    bool  notify_pm       = true;
    bool  notify_connect  = true;
    bool  notify_file     = true;
    bool  notify_server   = true;
    bool  notif_sound     = false;
    bool  notif_systray   = false;
    int   notif_timeout   = 4000;
    int   msg_limit       = 2000;

    // Net diag
    char  diag_host[128]  = "127.0.0.1";
    int   diag_port       = 9000;
    char  diag_result[1024] = "Press a test button.";
} g_nav;

// ── Quick helpers ──────────────────────────────────────────────
static void accent(const char* s) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.60f,0.15f,1.0f));
    ImGui::Text("%s", s);
    ImGui::PopStyleColor();
}
static void hsep() {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.22f,0.22f,0.24f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
}
static void mhdr(const char* t) {
    ImGui::Spacing(); accent(t); hsep(); ImGui::Spacing();
}



// ── About modal ──────────────────────────────────────────────────────────────
static void draw_about_modal(AppState& app)
{
    if (!app.show_about) return;
    ImGui::OpenPopup("About SafeSocket");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(440, 280), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("About SafeSocket", &app.show_about,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.60f, 0.15f, 1.0f));
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("SafeSocket");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("v0.0.2");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped("Open-source encrypted TCP socket communication tool.");
        ImGui::TextWrapped("Supports XOR, Vigenere and RC4 stream encryption.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("MIT License");
        ImGui::TextDisabled("github.com/PanagiotisKotsorgios/SafeSocket");
        ImGui::TextDisabled("Built with Dear ImGui + SDL3 + OpenGL 3.3");
        ImGui::Spacing();
        float bw = 90.0f;
        ImGui::SetCursorPosX((440.0f - bw) * 0.5f);
        if (ImGui::Button("Close", ImVec2(bw, 28))) app.show_about = false;
        ImGui::EndPopup();
    }
}

// ── Help modal ───────────────────────────────────────────────────────────────
static void draw_help_modal(AppState& app)
{
    if (!app.show_help) return;
    ImGui::OpenPopup("Help & Keybinds");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460, 300), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Help & Keybinds", &app.show_help,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.60f, 0.15f, 1.0f));
        ImGui::Text("Keyboard Shortcuts");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::BeginTable("##keys", 2,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Key",    ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            struct KB { const char* key; const char* action; };
            static const KB rows[] = {
                {"Tab",      "Navigate between fields"   },
                {"Enter",    "Send message (chat)"       },
                {"Ctrl + L", "Clear chat log"            },
                {"Ctrl + D", "Disconnect"                },
                {"Ctrl + T", "Toggle dark / light theme" },
                {"F1",       "Open this help window"     },
                {"Alt + F4", "Quit"                      },
            };
            for (int i = 0; i < 7; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.60f,0.15f,1.0f));
                ImGui::Text("%s", rows[i].key);
                ImGui::PopStyleColor();
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", rows[i].action);
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();
        float bw = 90.0f;
        ImGui::SetCursorPosX((460.0f - bw) * 0.5f);
        if (ImGui::Button("Close", ImVec2(bw, 28))) app.show_help = false;
        ImGui::EndPopup();
    }
}

// ── Navbar ───────────────────────────────────────────────────────────────────

// ============================================================
//  Modal: Hotkeys quick-reference
// ============================================================
static void draw_hotkeys_modal() {
    if (!g_nav.show_hotkeys) return;
    ImGui::OpenPopup("Keyboard Shortcuts");
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(400,300),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Keyboard Shortcuts",&g_nav.show_hotkeys,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        mhdr("GLOBAL SHORTCUTS");
        struct R{const char*k;const char*d;};
        R rows[]={{"Ctrl+T","Toggle theme"},{"Ctrl+N","Go to Connect"},
                  {"Ctrl+W","Disconnect/stop"},{"Ctrl+D","Database config"},
                  {"Ctrl+E","Export data"},{"Ctrl+L","Refresh client list"},
                  {"F1","Full Help"},{"F5","Refresh view"},{"Esc","Close modal"}};
        for(auto&r:rows){
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.85f,0.75f,0.35f,1.0f));
            ImGui::Text("  %-20s",r.k); ImGui::PopStyleColor();
            ImGui::SameLine(); ImGui::TextDisabled("%s",r.d);
        }
        ImGui::Spacing();
        if(ImGui::Button("Close##hkc",ImVec2(-1,28))) g_nav.show_hotkeys=false;
        ImGui::EndPopup();
    }
}

// ============================================================
//  Modal: Database Configuration
// ============================================================
static void draw_db_config_modal() {
    if (!g_nav.show_db_config) return;
    ImGui::OpenPopup("Database Configuration");
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(540,500),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Database Configuration",&g_nav.show_db_config,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        mhdr("ENGINE");
        const char* dbt[]={"None (disabled)","SQLite (local file)",
                            "MySQL / MariaDB","PostgreSQL","Redis"};
        ImGui::Text("Engine"); ImGui::SameLine(130.0f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##dbtype",&g_nav.db_type,dbt,5)) {
            // auto-set default port
            if(g_nav.db_type==2) g_nav.db_port=3306;
            if(g_nav.db_type==3) g_nav.db_port=5432;
            if(g_nav.db_type==4) g_nav.db_port=6379;
        }
        ImGui::Spacing();
        if (g_nav.db_type==1) {
            mhdr("SQLITE");
            ImGui::Text("File path"); ImGui::SameLine(130.0f);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##sqp",g_nav.db_sqlite_path,sizeof(g_nav.db_sqlite_path));
            ImGui::TextDisabled("  Created automatically if absent.");
        } else if (g_nav.db_type>=2) {
            mhdr("CONNECTION");
            float LW=130.0f;
            ImGui::Text("Host");     ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##dbh",g_nav.db_host,sizeof(g_nav.db_host));
            ImGui::Text("Port");     ImGui::SameLine(LW); ImGui::SetNextItemWidth(100.0f);
            ImGui::InputInt("##dbp",&g_nav.db_port,1,100);
            if(g_nav.db_port<1) g_nav.db_port=1;
            ImGui::Text("Database"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##dbn",g_nav.db_name,sizeof(g_nav.db_name));
            ImGui::Text("User");     ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##dbu",g_nav.db_user,sizeof(g_nav.db_user));
            ImGui::Text("Password"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##dbpw",g_nav.db_pass,sizeof(g_nav.db_pass),
                             ImGuiInputTextFlags_Password);
            ImGui::Text("TLS/SSL");  ImGui::SameLine(LW);
            ImGui::Checkbox("##dbs",&g_nav.db_ssl);
            mhdr("POOL");
            ImGui::Text("Pool size");  ImGui::SameLine(LW); ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderInt("##dbps",&g_nav.db_pool_size,1,32);
            ImGui::Text("Timeout(s)"); ImGui::SameLine(LW); ImGui::SetNextItemWidth(120.0f);
            ImGui::SliderInt("##dbt",&g_nav.db_timeout_sec,1,60);
        } else {
            ImGui::TextDisabled("  Select an engine to configure.");
        }
        ImGui::Spacing(); hsep(); ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text,
            g_nav.db_connected ? ImVec4(0.35f,0.80f,0.35f,1.0f)
                               : ImVec4(0.65f,0.40f,0.40f,1.0f));
        ImGui::Text("Status: %s", g_nav.db_status);
        ImGui::PopStyleColor();
        ImGui::Spacing();
        if (g_nav.db_type>0) {
            if (ImGui::Button("Test##dbtest",ImVec2(110,28))) {
                if(g_nav.db_type==1)
                    snprintf(g_nav.db_status,sizeof(g_nav.db_status),
                             "SQLite: would open '%.180s'",g_nav.db_sqlite_path);
                else
                    snprintf(g_nav.db_status,sizeof(g_nav.db_status),
                             "Connect %.60s:%d/%.60s (stub)",
                             g_nav.db_host,g_nav.db_port,g_nav.db_name);
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Config##dbsave",ImVec2(110,28)))
                snprintf(g_nav.db_status,sizeof(g_nav.db_status),"Config saved (pending).");
            ImGui::SameLine();
        }
        if (ImGui::Button("Close##dbc",ImVec2(90,28))) g_nav.show_db_config=false;
        ImGui::EndPopup();
    }
}

// ============================================================
//  Modal: Export Data
// ============================================================
static void draw_export_modal(AppState& app) {
    if (!g_nav.show_export) return;
    ImGui::OpenPopup("Export Data");
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(420,360),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Export Data",&g_nav.show_export,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        mhdr("FORMAT");
        const char* fmts[]={"JSON","CSV","XML","SQL dump"};
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##expfmt",&g_nav.export_fmt,fmts,4);
        mhdr("OUTPUT PATH");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##expp",g_nav.export_path,sizeof(g_nav.export_path));
        const char* ext[]={"json","csv","xml","sql"};
        ImGui::TextDisabled("  Output: %s.%s",g_nav.export_path,ext[g_nav.export_fmt]);
        mhdr("INCLUDE");
        ImGui::Checkbox("Chat messages",    &g_nav.export_messages);
        ImGui::Checkbox("File transfers",   &g_nav.export_transfers);
        ImGui::Checkbox("Connection log",   &g_nav.export_log);
        ImGui::Checkbox("Configuration",    &g_nav.export_config);
        ImGui::Spacing(); hsep(); ImGui::Spacing();
        int n = (g_nav.export_messages?(int)app.messages.size():0)
              + (g_nav.export_transfers?(int)app.transfers.size():0)
              + (g_nav.export_log?(int)app.conn_log.size():0);
        ImGui::TextDisabled("  Estimated records: %d",n);
        ImGui::Spacing();
        if (ImGui::Button("Export##doexp",ImVec2(110,28))) {
            app.log("Export: "+std::string(g_nav.export_path)+
                    "."+std::string(ext[g_nav.export_fmt]));
            app.set_status("Export complete (stub).");
            g_nav.show_export=false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##expc",ImVec2(90,28))) g_nav.show_export=false;
        ImGui::EndPopup();
    }
}

// ============================================================
//  Modal: Network Diagnostics
// ============================================================
static void draw_net_diag_modal() {
    if (!g_nav.show_net_diag) return;
    ImGui::OpenPopup("Network Diagnostics");
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(460,320),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Network Diagnostics",&g_nav.show_net_diag,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        mhdr("TARGET");
        ImGui::Text("Host"); ImGui::SameLine(70.0f);
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("##dgh",g_nav.diag_host,sizeof(g_nav.diag_host));
        ImGui::SameLine(); ImGui::Text("Port"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("##dgp",&g_nav.diag_port,0,0);
        ImGui::Spacing();
        if (ImGui::Button("TCP Test",ImVec2(110,28)))
            snprintf(g_nav.diag_result,sizeof(g_nav.diag_result),
                     "TCP test -> %s:%d\n(socket layer not wired in diag mode)",
                     g_nav.diag_host,g_nav.diag_port);
        ImGui::SameLine();
        if (ImGui::Button("DNS Lookup",ImVec2(110,28)))
            snprintf(g_nav.diag_result,sizeof(g_nav.diag_result),
                     "DNS: '%s' -> would call getaddrinfo()",g_nav.diag_host);
        ImGui::SameLine();
        if (ImGui::Button("Clear##dgc",ImVec2(70,28)))
            snprintf(g_nav.diag_result,sizeof(g_nav.diag_result),"Press a test button.");
        ImGui::Spacing(); hsep(); mhdr("RESULT");
        ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0.06f,0.06f,0.07f,1.0f));
        ImGui::BeginChild("##dgo",ImVec2(-1,90),true);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.65f,0.85f,0.55f,1.0f));
        ImGui::TextWrapped("%s",g_nav.diag_result);
        ImGui::PopStyleColor(); ImGui::EndChild(); ImGui::PopStyleColor();
        ImGui::Spacing();
        if (ImGui::Button("Close##dgx",ImVec2(-1,28))) g_nav.show_net_diag=false;
        ImGui::EndPopup();
    }
}

// ============================================================
//  Modal: Log Viewer
// ============================================================
static void draw_log_viewer_modal(AppState& app) {
    if (!g_nav.show_log_viewer) return;
    ImGui::OpenPopup("Log Viewer");
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(620,460),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Log Viewer",&g_nav.show_log_viewer,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        mhdr("CONNECTION / SYSTEM LOG");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x-90.0f);
        if (ImGui::SmallButton(" Clear Log ")) {
            std::lock_guard<std::mutex> lk(app.conn_log_mutex);
            app.conn_log.clear();
        }
        ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0.06f,0.06f,0.07f,1.0f));
        ImGui::BeginChild("##lv",ImVec2(-1,-50),true,ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lk(app.conn_log_mutex);
            for (size_t i=0;i<app.conn_log.size();++i) {
                const std::string& e=app.conn_log[i];
                ImVec4 col=ImVec4(0.54f,0.54f,0.56f,1.0f);
                if(e.find("error")!=e.npos||e.find("fail")!=e.npos||e.find("FAIL")!=e.npos)
                    col=ImVec4(0.88f,0.40f,0.36f,1.0f);
                else if(e.find("connect")!=e.npos||e.find("start")!=e.npos||e.find("OK")!=e.npos)
                    col=ImVec4(0.55f,0.82f,0.55f,1.0f);
                else if(e.find("warn")!=e.npos)
                    col=ImVec4(0.85f,0.75f,0.30f,1.0f);
                ImGui::PushStyleColor(ImGuiCol_Text,col);
                ImGui::Text("[%03d] %s",(int)(i+1),e.c_str());
                ImGui::PopStyleColor();
            }
        }
        if(ImGui::GetScrollY()>=ImGui::GetScrollMaxY()-4.0f) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild(); ImGui::PopStyleColor();
        ImGui::Spacing();
        if(ImGui::Button("Export Log...",ImVec2(120,28))){
            g_nav.export_messages=false; g_nav.export_transfers=false;
            g_nav.export_log=true; g_nav.export_config=false;
            g_nav.show_export=true; g_nav.show_log_viewer=false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Close##lvc",ImVec2(90,28))) g_nav.show_log_viewer=false;
        ImGui::EndPopup();
    }
}

// ============================================================
//  Modal: Preferences
// ============================================================
static void draw_prefs_modal(AppState& app) {
    if (!g_nav.show_prefs) return;
    ImGui::OpenPopup("Preferences");
    ImVec2 ctr = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ctr,ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(460,490),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Preferences",&g_nav.show_prefs,
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
        const float LW=180.0f;
        mhdr("APPEARANCE");
        ImGui::Text("UI Scale");        ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(160.0f);
        if(ImGui::SliderFloat("##uisc",&g_nav.font_scale,0.7f,1.5f,"%.2fx"))
            ImGui::GetIO().FontGlobalScale=g_nav.font_scale;
        ImGui::Text("Compact mode");   ImGui::SameLine(LW);
        ImGui::Checkbox("##cmp",&g_nav.compact_mode);
        ImGui::Text("Show status bar"); ImGui::SameLine(LW);
        ImGui::Checkbox("##ssb",&g_nav.show_status_bar);
        ImGui::Text("Show footer");     ImGui::SameLine(LW);
        ImGui::Checkbox("##sft",&g_nav.show_footer);
        mhdr("CHAT");
        ImGui::Text("Show timestamps"); ImGui::SameLine(LW);
        ImGui::Checkbox("##sts",&g_nav.show_timestamps);
        ImGui::Text("Message limit");   ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputInt("##msl",&g_nav.msg_limit,100,500);
        if(g_nav.msg_limit<100) g_nav.msg_limit=100;
        if(g_nav.msg_limit>9999) g_nav.msg_limit=9999;
        mhdr("NOTIFICATIONS");
        ImGui::Text("Private messages"); ImGui::SameLine(LW);
        ImGui::Checkbox("##npm",&g_nav.notify_pm);
        ImGui::Text("Connections");      ImGui::SameLine(LW);
        ImGui::Checkbox("##nco",&g_nav.notify_connect);
        ImGui::Text("File transfers");   ImGui::SameLine(LW);
        ImGui::Checkbox("##nfi",&g_nav.notify_file);
        ImGui::Text("Server events");    ImGui::SameLine(LW);
        ImGui::Checkbox("##nse",&g_nav.notify_server);
        ImGui::Text("Sound");            ImGui::SameLine(LW);
        ImGui::Checkbox("##nso",&g_nav.notif_sound);
        ImGui::SameLine(); ImGui::TextDisabled("(audio backend req.)");
        ImGui::Text("Timeout (ms)");     ImGui::SameLine(LW);
        ImGui::SetNextItemWidth(120.0f);
        ImGui::SliderInt("##nto",&g_nav.notif_timeout,1000,10000);
        ImGui::Spacing(); hsep(); ImGui::Spacing();
        if(ImGui::Button("Apply##pa",ImVec2(100,28))){
            app.log("Preferences applied."); g_nav.show_prefs=false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel##pc",ImVec2(90,28))) g_nav.show_prefs=false;
        ImGui::EndPopup();
    }
}


// ============================================================
//  draw_navbar — full application menu bar
// ============================================================
void draw_navbar(AppState& app)
{
    if (!ImGui::BeginMenuBar()) return;

    // Brand
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f,0.60f,0.15f,1.0f));
    ImGui::Text("SafeSocket"); ImGui::PopStyleColor();
    ImGui::TextDisabled(" v0.0.2");
    ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();

    // ── FILE ────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("File")) {
        if (ImGui::BeginMenu("New Connection")) {
            if (ImGui::MenuItem("Client Connection", "Ctrl+N")) app.screen = AppScreen::Connect;
            if (ImGui::MenuItem("Server Instance"))              app.screen = AppScreen::Connect;
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Profiles")) {
            if (ImGui::MenuItem("Load Profile..."))   app.log("Profile load (placeholder).");
            if (ImGui::MenuItem("Save Profile..."))   app.log("Profile save (placeholder).");
            if (ImGui::MenuItem("Delete Profile...")) app.log("Profile delete (placeholder).");
            ImGui::Separator();
            if (ImGui::MenuItem("Import CLI .conf...")) app.log("CLI .conf import (placeholder).");
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Export")) {
            if (ImGui::MenuItem("Export All...",        "Ctrl+E")) { g_nav.show_export=true; }
            if (ImGui::MenuItem("Export Chat Log..."))  { g_nav.export_messages=true;  g_nav.export_transfers=false; g_nav.export_log=false; g_nav.show_export=true; }
            if (ImGui::MenuItem("Export Transfers...")) { g_nav.export_messages=false; g_nav.export_transfers=true;  g_nav.export_log=false; g_nav.show_export=true; }
            if (ImGui::MenuItem("Export Config..."))    { g_nav.export_messages=false; g_nav.export_transfers=false; g_nav.export_config=true; g_nav.show_export=true; }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Generate safesocket.conf")) {
            g_config.save("safesocket.conf");
            app.set_status("safesocket.conf written.");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Alt+F4")) {
            SDL_Event e; e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e);
        }
        ImGui::EndMenu();
    }

    // ── CONNECTION ──────────────────────────────────────────────────
    if (ImGui::BeginMenu("Connection")) {
        bool cli = app.client_connected.load();
        bool srv = app.server_running.load();
        if (ImGui::BeginMenu("Client")) {
            if (cli) { if (ImGui::MenuItem("Disconnect","Ctrl+W")) app.do_disconnect(); }
            else     { if (!srv && ImGui::MenuItem("Connect...","Ctrl+N")) app.screen=AppScreen::Connect; }
            ImGui::Separator();
            ImGui::MenuItem("Auto-reconnect", nullptr, &app.cs_auto_reconnect);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Server")) {
            if (srv) { if (ImGui::MenuItem("Stop Server","Ctrl+W")) app.do_stop_server(); }
            else     { if (!cli && ImGui::MenuItem("Start Server...")) app.screen=AppScreen::Connect; }
            ImGui::Separator();
            if (ImGui::MenuItem("Server Config...")) app.screen=AppScreen::Connect;
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Encryption")) {
            const char* t[]={"None","XOR","Vigenere","RC4"};
            for(int i=0;i<4;++i)
                if(ImGui::MenuItem(t[i],nullptr,app.cs_encrypt==i)) app.cs_encrypt=i;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Network Diagnostics")) {
            if (ImGui::MenuItem("Open Diagnostics...")) g_nav.show_net_diag=true;
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Request Client List","Ctrl+L")) { app.request_list(); app.log("List requested."); }
        ImGui::EndMenu();
    }

    // ── VIEW ────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Connect / Home","Ctrl+N"))                                    app.screen=AppScreen::Connect;
        if (ImGui::MenuItem("Client View",  nullptr, false, app.client_connected.load())) app.screen=AppScreen::Client;
        if (ImGui::MenuItem("Server View",  nullptr, false, app.server_running.load()))   app.screen=AppScreen::Server;
        ImGui::Separator();
        if (ImGui::MenuItem("Log Viewer..."))  g_nav.show_log_viewer=true;
        ImGui::Separator();
        if (ImGui::MenuItem("Show Status Bar", nullptr, g_nav.show_status_bar)) g_nav.show_status_bar=!g_nav.show_status_bar;
        if (ImGui::MenuItem("Show Footer",     nullptr, g_nav.show_footer))     g_nav.show_footer=!g_nav.show_footer;
        ImGui::EndMenu();
    }

    // ── SETTINGS ────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Preferences...","Ctrl+,")) g_nav.show_prefs=true;
        ImGui::Separator();
        if (ImGui::BeginMenu("Appearance")) {
            if (ImGui::MenuItem("Dark Theme",  "Ctrl+T", app.dark_theme))  { app.dark_theme=true;  apply_dark_theme(); }
            if (ImGui::MenuItem("Light Theme", nullptr, !app.dark_theme))  { app.dark_theme=false; apply_light_theme(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Compact Mode",   nullptr, g_nav.compact_mode))    g_nav.compact_mode=!g_nav.compact_mode;
            if (ImGui::MenuItem("Show Timestamps", nullptr, g_nav.show_timestamps)) g_nav.show_timestamps=!g_nav.show_timestamps;
            ImGui::Separator();
            if (ImGui::MenuItem("Scale 80%"))  { g_nav.font_scale=0.8f; ImGui::GetIO().FontGlobalScale=0.8f; }
            if (ImGui::MenuItem("Scale 100%")) { g_nav.font_scale=1.0f; ImGui::GetIO().FontGlobalScale=1.0f; }
            if (ImGui::MenuItem("Scale 120%")) { g_nav.font_scale=1.2f; ImGui::GetIO().FontGlobalScale=1.2f; }
            if (ImGui::MenuItem("Scale 150%")) { g_nav.font_scale=1.5f; ImGui::GetIO().FontGlobalScale=1.5f; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Transfer")) {
            if (ImGui::MenuItem("Auto-accept files", nullptr, app.set_auto_accept))
                app.set_auto_accept=!app.set_auto_accept;
            ImGui::Separator();
            ImGui::TextDisabled("  Download dir:");
            ImGui::SetNextItemWidth(220.0f);
            ImGui::InputText("##nddl",app.set_download_dir,sizeof(app.set_download_dir));
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Logging")) {
            if (ImGui::MenuItem("Log to file",    nullptr, app.set_log_to_file)) app.set_log_to_file=!app.set_log_to_file;
            if (ImGui::MenuItem("Verbose output", nullptr, app.set_verbose))     app.set_verbose=!app.set_verbose;
            ImGui::Separator();
            if (ImGui::MenuItem("View Log...")) g_nav.show_log_viewer=true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Notifications")) {
            if (ImGui::MenuItem("On private message",  nullptr, g_nav.notify_pm))      g_nav.notify_pm=!g_nav.notify_pm;
            if (ImGui::MenuItem("On connection event", nullptr, g_nav.notify_connect))  g_nav.notify_connect=!g_nav.notify_connect;
            if (ImGui::MenuItem("On file transfer",    nullptr, g_nav.notify_file))    g_nav.notify_file=!g_nav.notify_file;
            if (ImGui::MenuItem("On server event",     nullptr, g_nav.notify_server))  g_nav.notify_server=!g_nav.notify_server;
            ImGui::Separator();
            if (ImGui::MenuItem("Configure...")) g_nav.show_prefs=true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Keepalive")) {
            if (ImGui::MenuItem("Enable keepalive", nullptr, app.set_keepalive)) app.set_keepalive=!app.set_keepalive;
            ImGui::Separator();
            ImGui::TextDisabled("  Ping interval (sec):");
            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderInt("##kpivl",&app.set_ping_interval,5,120);
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save to safesocket.conf")) {
            app.apply_settings(); g_config.save("safesocket.conf");
            app.set_status("Settings saved.");
        }
        if (ImGui::MenuItem("Load from safesocket.conf")) {
            app.load_settings_from_config();
            app.set_status("Settings loaded.");
        }
        ImGui::EndMenu();
    }

    // ── DATABASE ────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Database")) {
        if (ImGui::MenuItem("Configure Connection...","Ctrl+D")) g_nav.show_db_config=true;
        ImGui::Separator();
        if (ImGui::BeginMenu("Engine")) {
            const char* e[]={"None","SQLite","MySQL","PostgreSQL","Redis"};
            for(int i=0;i<5;++i)
                if(ImGui::MenuItem(e[i],nullptr,g_nav.db_type==i)) g_nav.db_type=i;
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Test Connection")) {
            g_nav.show_db_config=true;
            snprintf(g_nav.db_status,sizeof(g_nav.db_status),"Test from menu...");
        }
        if (ImGui::MenuItem("Reset / Disconnect")) {
            g_nav.db_connected=false;
            snprintf(g_nav.db_status,sizeof(g_nav.db_status),"Not connected");
            app.log("DB reset.");
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Schema (future)")) {
            ImGui::TextDisabled("  Run Migrations");
            ImGui::TextDisabled("  View Tables");
            ImGui::TextDisabled("  Export Schema");
            ImGui::TextDisabled("  Reset / Drop All");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Backup (future)")) {
            ImGui::TextDisabled("  Backup Now...");
            ImGui::TextDisabled("  Restore...");
            ImGui::TextDisabled("  Schedule Auto-Backup");
            ImGui::EndMenu();
        }
        ImGui::Separator();
        {
            const char* eng[]={"None","SQLite","MySQL","PostgreSQL","Redis"};
            ImGui::PushStyleColor(ImGuiCol_Text,
                g_nav.db_connected ? ImVec4(0.35f,0.80f,0.35f,1.0f) : ImVec4(0.58f,0.38f,0.38f,1.0f));
            ImGui::Text("  [%s] %s", g_nav.db_connected?"LIVE":"OFF",
                        eng[g_nav.db_type<5?g_nav.db_type:0]);
            ImGui::PopStyleColor();
        }
        ImGui::EndMenu();
    }

    // ── TOOLS ───────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Network Diagnostics..."))   g_nav.show_net_diag=true;
        if (ImGui::MenuItem("Log Viewer..."))             g_nav.show_log_viewer=true;
        if (ImGui::MenuItem("Export Data...","Ctrl+E"))  g_nav.show_export=true;
        ImGui::Separator();
        if (ImGui::BeginMenu("Encryption Tester (future)")) {
            ImGui::TextDisabled("  Encrypt/decrypt test string");
            ImGui::TextDisabled("  Compare cipher outputs");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Packet Inspector (future)")) {
            ImGui::TextDisabled("  Capture raw packets");
            ImGui::TextDisabled("  Decode header fields");
            ImGui::TextDisabled("  Replay packets");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Certificate Manager (future)")) {
            ImGui::TextDisabled("  Generate self-signed cert");
            ImGui::TextDisabled("  Import PEM / PFX");
            ImGui::TextDisabled("  View certificate chain");
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Generate Default Config")) {
            g_config.save("safesocket.conf"); app.set_status("Config generated.");
        }
        if (ImGui::MenuItem("Open Downloads Folder")) {
#ifdef _WIN32
            ShellExecuteA(NULL,"explore",g_config.download_dir.c_str(),NULL,NULL,SW_SHOWDEFAULT);
#else
            char cmd[600]; snprintf(cmd,600,"xdg-open \"%s\" 2>/dev/null &",g_config.download_dir.c_str());
            (void)system(cmd);
#endif
        }
        ImGui::EndMenu();
    }

    // ── SECURITY ────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Security")) {
        bool locked = app.sec.enabled;
        if (ImGui::BeginMenu("App Lock")) {
            if (locked) {
                if (ImGui::MenuItem("Disable App Lock")) app.sec.enabled=false;
                if (ImGui::MenuItem("Lock Now"))         app.sec.authenticated=false;
            } else {
                if (ImGui::MenuItem("Enable App Lock")) { app.sec.enabled=true; app.sec.authenticated=false; }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Security Settings..."))  app.sec.show_security_menu=true;
            if (ImGui::MenuItem("Change Password..."))    app.sec.show_change_pass=true;
            if (ImGui::MenuItem("Export Credentials...")) app.sec.show_export=true;
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Encryption Policy")) {
            if (ImGui::MenuItem("Enforce RC4")) { app.cs_encrypt=3; app.sv_encrypt=3; app.log("Policy: RC4."); }
            if (ImGui::MenuItem("Allow any cipher")) app.log("Policy: any.");
            if (ImGui::MenuItem("Plaintext only"))  { app.cs_encrypt=0; app.sv_encrypt=0; app.log("Policy: none."); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Access Control")) {
            if (ImGui::MenuItem("Require access key (client)", nullptr, app.cs_require_key)) app.cs_require_key=!app.cs_require_key;
            if (ImGui::MenuItem("Require access key (server)", nullptr, app.sv_require_key)) app.sv_require_key=!app.sv_require_key;
            ImGui::Separator();
            ImGui::TextDisabled("  IP allowlist (future)");
            ImGui::TextDisabled("  IP blocklist (future)");
            ImGui::TextDisabled("  Rate limiting (future)");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Audit Log (future)")) {
            ImGui::TextDisabled("  View audit events");
            ImGui::TextDisabled("  Export audit log");
            ImGui::TextDisabled("  Configure retention");
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    // ── HELP ────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Full Help & Commands","F1")) app.show_help=true;
        if (ImGui::MenuItem("Keyboard Shortcuts...",""))  g_nav.show_hotkeys=true;
        ImGui::Separator();
        if (ImGui::BeginMenu("Documentation")) {
            if (ImGui::MenuItem("Protocol Reference"))    app.log("Docs: Protocol");
            if (ImGui::MenuItem("Config Guide"))          app.log("Docs: Config");
            if (ImGui::MenuItem("Encryption Guide"))      app.log("Docs: Encryption");
            if (ImGui::MenuItem("Database Integration"))  app.log("Docs: Database");
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("GitHub Repository")) {
#ifdef _WIN32
            ShellExecuteA(NULL,"open","https://github.com/PanagiotisKotsorgios/SafeSocket",NULL,NULL,SW_SHOWDEFAULT);
#else
            (void)system("xdg-open https://github.com/PanagiotisKotsorgios/SafeSocket 2>/dev/null &");
#endif
        }
        if (ImGui::MenuItem("Report a Bug")) {
#ifdef _WIN32
            ShellExecuteA(NULL,"open","https://github.com/PanagiotisKotsorgios/SafeSocket/issues",NULL,NULL,SW_SHOWDEFAULT);
#else
            (void)system("xdg-open https://github.com/PanagiotisKotsorgios/SafeSocket/issues 2>/dev/null &");
#endif
        }
        ImGui::Separator();
        if (ImGui::MenuItem("About SafeSocket...")) app.show_about=true;
        ImGui::EndMenu();
    }

    // ── Right side ─────────────────────────────────────────────────
    {
        float rw = 290.0f;
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - rw);

        bool cli = app.client_connected.load();
        bool srv = app.server_running.load();
        const char* st  = cli ? "Client OK" : srv ? "Server ON" : "Offline";
        ImVec4 sc = cli ? ImVec4(0.34f,0.75f,0.34f,1.0f) :
                    srv ? ImVec4(0.35f,0.55f,0.90f,1.0f) :
                          ImVec4(0.44f,0.44f,0.46f,1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text,sc); ImGui::Text("%s",st); ImGui::PopStyleColor();
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();

        const char* dbs_eng[]={"---","SQLite","MySQL","PgSQL","Redis"};
        ImGui::PushStyleColor(ImGuiCol_Text,
            g_nav.db_connected ? ImVec4(0.35f,0.80f,0.35f,1.0f) : ImVec4(0.44f,0.44f,0.46f,1.0f));
        ImGui::Text("DB:%s", g_nav.db_connected ? "LIVE" : dbs_eng[g_nav.db_type<5?g_nav.db_type:0]);
        ImGui::PopStyleColor();
        ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f,0.20f,0.22f,0.60f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.16f,0.16f,0.18f,0.60f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.55f,0.55f,0.57f,1.0f));
        if (ImGui::SmallButton(app.dark_theme ? "Light" : "Dark ")) {
            app.dark_theme=!app.dark_theme;
            if(app.dark_theme) apply_dark_theme(); else apply_light_theme();
        }
        ImGui::PopStyleColor(4);
    }
    ImGui::EndMenuBar();
}

// ============================================================
//  draw_host_window
// ============================================================
static void draw_host_window(AppState& app)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f,0.0f));
    ImGui::Begin("##host", NULL,
        ImGuiWindowFlags_NoDecoration          |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoScrollbar           |
        ImGuiWindowFlags_NoSavedSettings       |
        ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(3);

    // Security gate
    if (app.sec.show_wizard && !app.sec.wiz_password_shown) {
        draw_wizard_screen(app.sec); ImGui::End(); return;
    }
    if (app.sec.enabled && !app.sec.authenticated) {
        draw_login_screen(app.sec); ImGui::End(); return;
    }

    draw_navbar(app);

    // Status bar
    if (g_nav.show_status_bar && !app.status_msg.empty()) {
        ImVec4 bg  = app.status_is_error ? ImVec4(0.18f,0.06f,0.06f,1.0f) : ImVec4(0.11f,0.10f,0.08f,1.0f);
        ImVec4 col = app.status_is_error ? ImVec4(0.90f,0.44f,0.40f,1.0f) : ImVec4(0.85f,0.60f,0.15f,1.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg,bg);
        ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
        ImGui::BeginChild("##sb",ImVec2(0.0f,26.0f),false,
            ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(ImVec2(14.0f,5.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,col);
        ImGui::TextUnformatted(app.status_msg.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x-14.0f);
        ImGui::SetCursorPosY(3.0f);
        ImGui::PushStyleColor(ImGuiCol_Text,         ImVec4(0.40f,0.40f,0.42f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,       ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.24f,0.24f,0.26f,1.0f));
        if(ImGui::SmallButton(" x ")) app.status_msg.clear();
        ImGui::PopStyleColor(3);
        ImGui::EndChild(); ImGui::PopStyleColor(2);
        ImGui::PushStyleColor(ImGuiCol_Separator,ImVec4(0.18f,0.18f,0.20f,1.0f));
        ImGui::Separator(); ImGui::PopStyleColor();
    }

    // Content area
    float footer_h  = g_nav.show_footer ? 52.0f : 0.0f;
    float content_h = ImGui::GetContentRegionAvail().y - footer_h;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::BeginChild("##ca",ImVec2(0.0f,content_h),false,
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    switch (app.screen) {
    case AppScreen::Connect:
        draw_connect_window(app); break;
    case AppScreen::Client:
        if(ImGui::BeginTabBar("##ct")) {
            if(ImGui::BeginTabItem(" Chat "))     { draw_chat_window(app);     ImGui::EndTabItem(); }
            if(ImGui::BeginTabItem(" Files "))    { draw_files_window(app);    ImGui::EndTabItem(); }
            if(ImGui::BeginTabItem(" Settings ")) { draw_settings_window(app); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        } break;
    case AppScreen::Server:
        if(ImGui::BeginTabBar("##st")) {
            if(ImGui::BeginTabItem(" Clients "))   { draw_server_window(app);   ImGui::EndTabItem(); }
            if(ImGui::BeginTabItem(" Chat/Log "))  { draw_chat_window(app);     ImGui::EndTabItem(); }
            if(ImGui::BeginTabItem(" Files "))     { draw_files_window(app);    ImGui::EndTabItem(); }
            if(ImGui::BeginTabItem(" Settings "))  { draw_settings_window(app); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        } break;
    }
    ImGui::EndChild();

    // Footer
    if (g_nav.show_footer) {
        bool dk = app.dark_theme;
        ImGui::PushStyleColor(ImGuiCol_Separator,
            dk ? ImVec4(0.22f,0.22f,0.24f,1.0f) : ImVec4(0.52f,0.50f,0.48f,1.0f));
        ImGui::Separator(); ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
            dk ? ImVec4(0.05f,0.05f,0.06f,1.0f) : ImVec4(0.82f,0.81f,0.79f,1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
        ImGui::BeginChild("##ft",ImVec2(0.0f,footer_h),false,
            ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        ImVec4 ft = dk ? ImVec4(0.36f,0.36f,0.38f,1.0f) : ImVec4(0.28f,0.27f,0.25f,1.0f);
        ImGui::SetCursorPos(ImVec2(14.0f,16.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,ft);
        ImGui::Text("SafeSocket (c) 2025  |  MIT License  |  v0.0.2");
        const char* scrn[]={"Connect","Client","Server"};
        const char* scr = scrn[(int)app.screen<3?(int)app.screen:0];
        const char* dbe[]={"off","SQLite","MySQL","PgSQL","Redis"};
        char rb[128];
        snprintf(rb,sizeof(rb),"%s  |  DB:%s  |  Open Source",scr,
                 g_nav.db_connected?"LIVE":dbe[g_nav.db_type<5?g_nav.db_type:0]);
        float rw2=ImGui::CalcTextSize(rb).x;
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()-rw2-14.0f,16.0f));
        ImGui::Text("%s",rb);
        ImGui::PopStyleColor();
        ImGui::EndChild(); ImGui::PopStyleColor();
    }

    // All modals
    draw_security_modal(app.sec);
    draw_about_modal(app);
    draw_help_modal(app);
    draw_hotkeys_modal();
    draw_db_config_modal();
    draw_export_modal(app);
    draw_net_diag_modal();
    draw_log_viewer_modal(app);
    draw_prefs_modal(app);
    ImGui::End();
}

// ============================================================
//  Entry point
// ============================================================
int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError()); return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "SafeSocket v0.0.2", 1280, 800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) { SDL_Log("Window failed: %s", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        SDL_Log("GL context failed: %s", SDL_GetError());
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }
    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    apply_dark_theme();
    AppState app;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)                   running=false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running=false;
        }
        // Global shortcuts
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_T)) {
            app.dark_theme=!app.dark_theme;
            if(app.dark_theme) apply_dark_theme(); else apply_light_theme();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) app.screen=AppScreen::Connect;
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W)) {
            if(app.client_connected) app.do_disconnect();
            else if(app.server_running) app.do_stop_server();
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) g_nav.show_db_config=true;
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_E)) g_nav.show_export=true;
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L)) app.request_list();
        if (ImGui::IsKeyPressed(ImGuiKey_F1)) app.show_help=true;
        if (ImGui::IsKeyPressed(ImGuiKey_F5)) app.request_list();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        draw_host_window(app);
        ImGui::Render();

        int w,h; SDL_GetWindowSize(window,&w,&h);
        glViewport(0,0,w,h);
        glClearColor(app.dark_theme?0.08f:0.89f,
                     app.dark_theme?0.08f:0.89f,
                     app.dark_theme?0.09f:0.88f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
