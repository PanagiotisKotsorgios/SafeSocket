/**
 * @file main_gui.cpp
 * @brief Entry point for safesocket-gui.
 *
 * Initialises SDL2, creates an OpenGL context, sets up Dear ImGui, then
 * runs the main render loop.  The loop calls app_poll_events() each frame
 * to drain adapter queues, then draws the appropriate top-level window
 * based on AppState::screen.
 *
 * Build requirements:
 *   - SDL2 2.28+
 *   - OpenGL 3.3+
 *   - Dear ImGui (vendored in third_party/imgui)
 */

#include "app.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
  #include <SDL_opengles2.h>
#else
  #include <SDL_opengl.h>
#endif

#include <cstdio>
#include <cstring>

// ── Helper: apply dark theme ──────────────────────────────────────────────────

static void apply_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    c[ImGuiCol_WindowBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ChildBg]           = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    c[ImGuiCol_PopupBg]           = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_Border]            = ImVec4(0.30f, 0.30f, 0.35f, 0.80f);
    c[ImGuiCol_FrameBg]           = ImVec4(0.17f, 0.17f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    c[ImGuiCol_TitleBg]           = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive]     = ImVec4(0.14f, 0.22f, 0.38f, 1.00f);
    c[ImGuiCol_MenuBarBg]         = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    c[ImGuiCol_ScrollbarBg]       = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
    c[ImGuiCol_ScrollbarGrab]     = ImVec4(0.30f, 0.30f, 0.36f, 0.80f);
    c[ImGuiCol_Button]            = ImVec4(0.20f, 0.35f, 0.60f, 1.00f);
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.26f, 0.44f, 0.76f, 1.00f);
    c[ImGuiCol_ButtonActive]      = ImVec4(0.16f, 0.28f, 0.50f, 1.00f);
    c[ImGuiCol_Header]            = ImVec4(0.20f, 0.35f, 0.60f, 0.80f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.26f, 0.44f, 0.76f, 0.90f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.16f, 0.28f, 0.50f, 1.00f);
    c[ImGuiCol_Separator]         = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
    c[ImGuiCol_Tab]               = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.26f, 0.44f, 0.76f, 0.90f);
    c[ImGuiCol_TabActive]         = ImVec4(0.20f, 0.35f, 0.60f, 1.00f);
    c[ImGuiCol_Text]              = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    c[ImGuiCol_TextDisabled]      = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    // ── SDL2 init ─────────────────────────────────────────────────────────────
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "[gui] SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    // GL 3.3 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags wflags = (SDL_WindowFlags)(
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    SDL_Window* window = SDL_CreateWindow(
        "SafeSocket v0.0.2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1100, 720,
        wflags
    );
    if (!window) {
        fprintf(stderr, "[gui] SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "[gui] SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // vsync

    // ── ImGui init ────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    apply_dark_theme();

    // Optionally load a custom font:
    // io.Fonts->AddFontFromFileTTF("../fonts/JetBrainsMono-Regular.ttf", 14.0f);

    // ── Application state ─────────────────────────────────────────────────────
    AppState app;

    // Apply any --config flag from argv
    for (int i = 1; i < argc - 1; ++i) {
        if (strcmp(argv[i], "--config") == 0) {
            g_config.load(argv[i + 1]);
            // Seed connect-screen defaults from loaded config
            strncpy(app.cs_host, g_config.host.c_str(),     sizeof(app.cs_host) - 1);
            strncpy(app.cs_nick, g_config.nickname.c_str(), sizeof(app.cs_nick) - 1);
            app.cs_port = g_config.port;
            break;
        }
    }

    // ── Main loop ─────────────────────────────────────────────────────────────
    bool running = true;
    while (running) {
        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        // Drain adapter queues
        app_poll_events(app);

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Full-screen docking area
        {
            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->Pos);
            ImGui::SetNextWindowSize(vp->Size);
            ImGui::SetNextWindowViewport(vp->ID);
            ImGuiWindowFlags host_flags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoSavedSettings;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::Begin("##host", nullptr, host_flags);
            ImGui::PopStyleVar(2);

            // Status banner
            if (!app.status_msg.empty()) {
                if (app.status_is_error)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                else
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));
                ImGui::TextUnformatted(app.status_msg.c_str());
                ImGui::PopStyleColor();
                ImGui::Separator();
            }

            // Route to the appropriate screen
            switch (app.screen) {
            case AppScreen::Connect:
                draw_connect_window(app);
                break;
            case AppScreen::Client:
                if (ImGui::BeginTabBar("##client_tabs")) {
                    if (ImGui::BeginTabItem("Chat"))        { draw_chat_window(app);     ImGui::EndTabItem(); }
                    if (ImGui::BeginTabItem("Files"))       { draw_files_window(app);    ImGui::EndTabItem(); }
                    if (ImGui::BeginTabItem("Settings"))    { draw_settings_window(app); ImGui::EndTabItem(); }
                    ImGui::EndTabBar();
                }
                break;
            case AppScreen::Server:
                if (ImGui::BeginTabBar("##server_tabs")) {
                    if (ImGui::BeginTabItem("Clients"))     { draw_server_window(app);   ImGui::EndTabItem(); }
                    if (ImGui::BeginTabItem("Chat / Log"))  { draw_chat_window(app);     ImGui::EndTabItem(); }
                    if (ImGui::BeginTabItem("Files"))       { draw_files_window(app);    ImGui::EndTabItem(); }
                    if (ImGui::BeginTabItem("Settings"))    { draw_settings_window(app); ImGui::EndTabItem(); }
                    ImGui::EndTabBar();
                }
                break;
            }

            ImGui::End();
        }

        // Render
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    if (app.screen == AppScreen::Client)
        app.gui_client.disconnect();
    if (app.screen == AppScreen::Server)
        app.gui_server.stop();

    net_cleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
