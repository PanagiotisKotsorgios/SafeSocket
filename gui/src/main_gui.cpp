#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_sdl3.h"
#include "../../imgui/imgui_impl_opengl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "../include/app.hpp"
#include "../include/security.hpp"

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
void draw_navbar(AppState& app)
{
    if (!ImGui::BeginMenuBar()) return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.60f, 0.15f, 1.0f));
    ImGui::Text("SafeSocket");
    ImGui::PopStyleColor();
    ImGui::TextDisabled("0.0.2");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Connect"))     app.screen = AppScreen::Connect;
        if (ImGui::MenuItem("Client View")) app.screen = AppScreen::Client;
        if (ImGui::MenuItem("Server View")) app.screen = AppScreen::Server;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Security")) {
        bool locked = app.sec.enabled;
        if (locked) {
            if (ImGui::MenuItem("Disable Lock")) app.sec.enabled = false;
        } else {
            if (ImGui::MenuItem("Enable Lock")) {
    app.sec.enabled = true;
    app.sec.authenticated = false; // 🔴 FORCE LOGIN
}
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Security Settings"))
            app.sec.show_security_menu = true;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Keybinds  [F1]")) app.show_help  = true;
        if (ImGui::MenuItem("About"))          app.show_about = true;
        ImGui::Separator();
        if (ImGui::MenuItem("GitHub"))
            (void)system("xdg-open https://github.com/PanagiotisKotsorgios/SafeSocket 2>/dev/null &");
        ImGui::EndMenu();
    }

    float rw = 200.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - rw);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f,0.20f,0.22f,0.60f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.16f,0.16f,0.18f,0.60f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.55f,0.55f,0.57f,1.0f));
    if (ImGui::SmallButton(app.dark_theme ? "Light Mode" : "Dark Mode")) {
        app.dark_theme = !app.dark_theme;
        if (app.dark_theme) apply_dark_theme();
        else                apply_light_theme();
    }
    ImGui::PopStyleColor(4);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    bool online = (app.screen != AppScreen::Connect);
    ImGui::PushStyleColor(ImGuiCol_Text, online
        ? ImVec4(0.34f, 0.62f, 0.28f, 1.0f)
        : ImVec4(0.46f, 0.46f, 0.48f, 1.0f));
    ImGui::Text(online ? "Connected" : "Offline");
    ImGui::PopStyleColor();

    ImGui::EndMenuBar();
}

// ── Host window ──────────────────────────────────────────────────────────────
static void draw_host_window(AppState& app)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));

    ImGui::Begin("##host", NULL,
        ImGuiWindowFlags_NoDecoration          |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoScrollbar           |
        ImGuiWindowFlags_NoSavedSettings       |
        ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(3);

    // ── Security gate — wizard and login render fullscreen over everything ──
    if (app.sec.show_wizard && !app.sec.wiz_password_shown) {
        draw_wizard_screen(app.sec);
        ImGui::End();
        return;
    }
    if (app.sec.enabled && !app.sec.authenticated) {
        draw_login_screen(app.sec);
        ImGui::End();
        return;
    }

    draw_navbar(app);

    // ── Status bar — fixed 24px strip, only when active ──────────────────
    if (!app.status_msg.empty()) {
        ImVec4 bg  = app.status_is_error
            ? ImVec4(0.18f, 0.06f, 0.06f, 1.0f)
            : ImVec4(0.11f, 0.10f, 0.08f, 1.0f);
        ImVec4 col = app.status_is_error
            ? ImVec4(0.90f, 0.44f, 0.40f, 1.0f)
            : ImVec4(0.85f, 0.60f, 0.15f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
        ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0,0,0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::BeginChild("##statusbar", ImVec2(0.0f, 26.0f), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();

        ImGui::SetCursorPos(ImVec2(14.0f, 9.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(app.status_msg.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 14.0f);
        ImGui::SetCursorPosY(4.0f);
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.40f,0.40f,0.42f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f,0.24f,0.26f,1.0f));
        if (ImGui::SmallButton(" x ")) app.status_msg.clear();
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::PopStyleColor(2);

        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.18f,0.18f,0.20f,1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

    // ── Content area: everything between navbar and footer ─────────────────
    // Reserve 26px for footer at the bottom
    float footer_h  = 52.0f;
    float content_h = ImGui::GetContentRegionAvail().y - footer_h;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::BeginChild("##content_area", ImVec2(0.0f, content_h), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    switch (app.screen) {
    case AppScreen::Connect:
        draw_connect_window(app);
        break;
    case AppScreen::Client:
        if (ImGui::BeginTabBar("##client_tabs")) {
            if (ImGui::BeginTabItem(" Chat "))     { draw_chat_window(app);     ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Files "))    { draw_files_window(app);    ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Settings ")) { draw_settings_window(app); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        break;
    case AppScreen::Server:
        if (ImGui::BeginTabBar("##server_tabs")) {
            if (ImGui::BeginTabItem(" Clients "))    { draw_server_window(app);   ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Chat / Log ")) { draw_chat_window(app);     ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Files "))      { draw_files_window(app);    ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem(" Settings "))   { draw_settings_window(app); ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
        break;
    }

    ImGui::EndChild(); // ##content_area

    // ── Footer — always rendered at the very bottom ────────────────────────
    bool is_dark = app.dark_theme;

    ImGui::PushStyleColor(ImGuiCol_Separator,
        is_dark ? ImVec4(0.22f,0.22f,0.24f,1.0f)
                : ImVec4(0.52f,0.50f,0.48f,1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        is_dark ? ImVec4(0.05f,0.05f,0.06f,1.0f)
                : ImVec4(0.82f,0.81f,0.79f,1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::BeginChild("##footer", ImVec2(0.0f, footer_h), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    ImVec4 footer_text = is_dark
        ? ImVec4(0.36f, 0.36f, 0.38f, 1.0f)
        : ImVec4(0.28f, 0.27f, 0.25f, 1.0f);

    ImGui::SetCursorPos(ImVec2(14.0f, 16.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, footer_text);
    ImGui::Text("SafeSocket (c) 2025   |   MIT License   |   v0.0.2");

    float rtext_w = ImGui::CalcTextSize("Open Source Project").x;
    ImGui::SetCursorPos(ImVec2(
        ImGui::GetWindowWidth() - rtext_w - 14.0f, 16.0f));
    ImGui::Text("Open Source Project");
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg

    // Modals always last
    draw_security_modal(app.sec);
    draw_about_modal(app);
    draw_help_modal(app);

    ImGui::End();
}

// ── Entry point ──────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "SafeSocket v0.0.2", 1200, 760,
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
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_T)) {
            app.dark_theme = !app.dark_theme;
            if (app.dark_theme) apply_dark_theme();
            else                apply_light_theme();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F1)) app.show_help = true;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        draw_host_window(app);

        ImGui::Render();
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(
            app.dark_theme ? 0.08f : 0.96f,
            app.dark_theme ? 0.08f : 0.96f,
            app.dark_theme ? 0.09f : 0.95f,
            1.0f);
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
