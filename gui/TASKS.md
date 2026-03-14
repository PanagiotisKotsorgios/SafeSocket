# SafeSocket GUI — Development Tasks

> **How to use this file**
> Work through tasks in order. Each task has a clear goal, exact files to touch,
> and a done-when checklist. Do not start task N+1 until every checkbox in task N
> is ticked. When a task is complete, mark it `[x]` and commit with the message
> format shown at the bottom of each task block.
>
> Tasks 1–8 bring the GUI to **full parity with the CLI** (v0.0.1 baseline).
> Tasks 9+ track each future version's GUI additions — one block per release.

---

## Legend

| Symbol | Meaning |
|---|---|
| `[ ]` | Not started |
| `[~]` | In progress |
| `[x]` | Done |
| 🔴 | Blocks the next task — must be done first |
| 🟡 | Important but does not block the next task |
| 🟢 | Polish / nice-to-have |

---
---

# PHASE 1 — Parity with CLI v0.0.1

---

## Task 1 — Shell, Theme & SDL2/ImGui Boilerplate

**Goal:** Get a black window on screen with the correct theme, correct window title,
and a clean SDL2 + OpenGL 3.3 + Dear ImGui render loop. Nothing functional yet —
just the canvas every other task will paint on.

**Files to create / touch**

| File | Action |
|---|---|
| `gui/src/main_gui.cpp` | Create from scratch (or start from the scaffold) |
| `gui/include/app.hpp` | Stub — declare `AppState` and `AppScreen` enum only |

**Step-by-step**

1. Initialise SDL2 with `SDL_INIT_VIDEO | SDL_INIT_TIMER`.
2. Request an OpenGL 3.3 Core Profile context:
   ```cpp
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   ```
3. Create a resizable window — initial size **1100 × 720**, title `"SafeSocket v0.0.2"`.
4. Call `ImGui::CreateContext()`, then `ImGui_ImplSDL2_InitForOpenGL()`, then `ImGui_ImplOpenGL3_Init("#version 330 core")`.
5. Enable keyboard navigation: `io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard`.
6. Apply the dark theme by calling `apply_dark_theme()` (see exact values below).
7. Clear to `(0.10, 0.10, 0.12, 1.0)` each frame — **same colour as WindowBg** so
   there is no seam between the host area and any windows.
8. Implement clean shutdown: on `SDL_QUIT` event call ImGui shutdown, delete GL
   context, destroy window, call `SDL_Quit()`.

**Exact theme values** (`apply_dark_theme()` in `main_gui.cpp`)

```cpp
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
```

**Host-window pattern** (fills the entire SDL window, no title bar, no resize grip)

```cpp
ImGuiViewport* vp = ImGui::GetMainViewport();
ImGui::SetNextWindowPos(vp->Pos);
ImGui::SetNextWindowSize(vp->Size);
ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
ImGui::Begin("##host", nullptr,
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoSavedSettings);
ImGui::PopStyleVar(2);
// ... draw content ...
ImGui::End();
```

**Done when**
- [ ] 🔴 Window opens. Background is `#1A1A1F` (the near-black defined above).
- [ ] 🔴 Window is resizable. Content fills the full SDL window with no gaps.
- [ ] 🔴 Closing the window exits cleanly with no console errors.
- [ ] 🟡 Title bar reads `SafeSocket v0.0.2`.
- [ ] 🟡 No ImGui demo window visible.

**Commit message:** `gui: task 1 — shell, theme, SDL2/ImGui boilerplate`

---

## Task 2 — AppState Skeleton & Screen Router

**Goal:** Define the central `AppState` struct and wire up the screen router so the
host window dispatches to the correct draw function based on `AppState::screen`.
No real content yet — each draw function just renders a placeholder text label.

**Files to create / touch**

| File | Action |
|---|---|
| `gui/include/app.hpp` | Full definition: `AppState`, `AppScreen`, `ChatMessage`, `FileTransferEntry` |
| `gui/src/app.cpp` | Create: `app_poll_events()` stub (just returns), forward declarations |
| `gui/src/window_connect.cpp` | Create: placeholder `draw_connect_window()` — render `"[Connect Screen]"` |
| `gui/src/window_chat.cpp` | Create: placeholder — render `"[Chat Window]"` |
| `gui/src/window_server.cpp` | Create: placeholder — render `"[Server Window]"` |
| `gui/src/window_files.cpp` | Create: placeholder — render `"[Files Window]"` |
| `gui/src/window_settings.cpp` | Create: placeholder — render `"[Settings Window]"` |
| `gui/src/main_gui.cpp` | Add the tab-bar router (see pattern below) |

**AppState fields required at this stage**

```cpp
struct AppState {
    AppScreen screen = AppScreen::Connect;

    // Chat history
    std::deque<ChatMessage> messages;
    static constexpr size_t MAX_MESSAGES = 2000;

    // File transfers
    std::vector<FileTransferEntry> transfers;

    // Status banner
    std::string status_msg;
    bool        status_is_error = false;

    void push_message(const std::string& sender, const std::string& text,
                      bool is_private = false, bool is_system = false);
    void set_status(const std::string& msg, bool error = false);
};
```

**Screen router pattern** (inside the `##host` window)

```cpp
switch (app.screen) {
case AppScreen::Connect:
    draw_connect_window(app);
    break;
case AppScreen::Client:
    if (ImGui::BeginTabBar("##client_tabs")) {
        if (ImGui::BeginTabItem("Chat"))     { draw_chat_window(app);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Files"))    { draw_files_window(app);    ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Settings")) { draw_settings_window(app); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    break;
case AppScreen::Server:
    if (ImGui::BeginTabBar("##server_tabs")) {
        if (ImGui::BeginTabItem("Clients"))    { draw_server_window(app);   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Chat / Log")) { draw_chat_window(app);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Files"))      { draw_files_window(app);    ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Settings"))   { draw_settings_window(app); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    break;
}
```

**Status banner** — render this at the top of the host window, just above the
router, every frame when `app.status_msg` is non-empty:

```cpp
if (!app.status_msg.empty()) {
    ImVec4 col = app.status_is_error
        ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)   // red
        : ImVec4(0.4f, 0.9f, 0.4f, 1.0f);  // green
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(app.status_msg.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
}
```

**Done when**
- [ ] 🔴 Each placeholder draw function is called when the matching `AppScreen` is active.
- [ ] 🔴 Tab bar appears in Client and Server modes (even with placeholder content).
- [ ] 🔴 `app.set_status("hello")` causes green text to appear at the top; `set_status("err", true)` causes red text.
- [ ] 🟡 Switching `app.screen` manually in code (temporary debug line) changes what is shown.

**Commit message:** `gui: task 2 — AppState skeleton and screen router`

---

## Task 3 — Connect Screen

**Goal:** Replace the connect-screen placeholder with the real form. By the end of
this task a developer can fill in the fields and press the buttons — the buttons
don't connect yet (that comes in Task 4), but all input state must be correctly
wired into `AppState`.

**Implements CLI equivalent:** `safesocket server [options]` / `safesocket client [options]`
(the argument-parsing step — collecting host, port, nick, encryption, key).

**Files to touch**

| File | Action |
|---|---|
| `gui/src/window_connect.cpp` | Full implementation |
| `gui/include/app.hpp` | Add connect-screen input fields to `AppState` |

**Fields to add to AppState**

```cpp
// Connect screen inputs
char cs_host[128]      = "127.0.0.1";
int  cs_port           = 9000;
char cs_nick[64]       = "anonymous";
int  cs_encrypt        = 0;   // 0=NONE 1=XOR 2=VIGENERE 3=RC4
char cs_key[256]       = {};
char cs_access_key[256]= {};
bool cs_require_key    = false;
```

**Layout specification**

Centre a panel of size **480 × 440** on screen (`ImGui::SetNextWindowPos` using
`(DisplaySize - panel_size) * 0.5f`). Use `ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove`.

```
┌────────────────────────────────────────┐
│  SafeSocket v0.0.2          [cyan bold]│
│  ──────────────────────────────────────│
│  Host      [ 127.0.0.1              ]  │
│  Port      [ 9000                   ]  │
│  ──────────────────────────────────────│
│  Nickname  [ anonymous              ]  │
│  ──────────────────────────────────────│
│  Encryption  [NONE ▼]                  │
│  Key         [••••••] (hidden if NONE) │
│  ──────────────────────────────────────│
│  ☐ Require access key (server only)    │
│  Access key  [••••••] (if checked)     │
│  ──────────────────────────────────────│
│  [ Connect as Client ] [ Start Server ]│
└────────────────────────────────────────┘
```

- Label column is 100 px wide; inputs fill the rest (`ImGui::SetNextItemWidth(-1)`).
- Encryption combo items: `{ "NONE", "XOR", "VIGENERE", "RC4" }`.
- Key field: `ImGuiInputTextFlags_Password`. Only visible when `cs_encrypt != 0`.
- Access key field: `ImGuiInputTextFlags_Password`. Only visible when `cs_require_key == true`.
- **Connect as Client** button: green tint (`ImVec4(0.18, 0.42, 0.18, 1.0)`), height 38 px.
- **Start Server** button: red tint (`ImVec4(0.42, 0.18, 0.18, 1.0)`), height 38 px.
- Both buttons are each half the panel width minus padding.
- On press: call `app.set_status("Not implemented yet")` — real logic in Task 4.

**Done when**
- [ ] 🔴 Panel is centred and does not move when the window is resized.
- [ ] 🔴 All 6 input fields are present and editable.
- [ ] 🔴 Key field appears/disappears correctly based on cipher selection.
- [ ] 🔴 Access key field appears/disappears based on the checkbox.
- [ ] 🔴 Both buttons are present and show the status banner when pressed.
- [ ] 🟡 Port field clamps to `[1, 65535]`.
- [ ] 🟡 Pressing Enter in any field does not submit the form (Tab to navigate).

**Commit message:** `gui: task 3 — connect screen layout and input fields`

---

## Task 4 — Networking: Connect & Disconnect

**Goal:** Wire the connect-screen buttons to real network operations.
`GuiClient` and `GuiServer` adapters must be fully implemented so that by the
end of this task the GUI can actually connect to a running CLI server and
exchange packets.

**Implements CLI equivalent:** `safesocket client --host … --port … --nick …`
and `safesocket server --port …` (the actual `connect()` / `start()` calls).

**Files to touch**

| File | Action |
|---|---|
| `gui/include/gui_client.hpp` | Full declaration |
| `gui/include/gui_server.hpp` | Full declaration |
| `gui/src/app.cpp` | Full implementation of `GuiClient` and `GuiServer` methods |
| `gui/src/window_connect.cpp` | Wire button callbacks to adapter calls |

**GuiClient — required public methods**

```cpp
bool connect(const std::string& host, int port);
void disconnect();
bool is_connected() const;          // wraps Client::is_connected()
uint32_t my_id() const;             // wraps Client::my_id()
std::vector<PeerInfo> peers() const;// wraps Client peer cache
void send_broadcast(const std::string& text);
void send_private(uint32_t peer_id, const std::string& text);
void send_file(uint32_t peer_id, const std::string& path);
void send_file_to_server(const std::string& path);
void send_nick_change(const std::string& new_nick);
bool poll(GuiClientEvent& ev);      // drain one event from queue
```

**GuiServer — required public methods**

```cpp
bool start(const std::string& host, int port);
void stop();
bool is_running() const;
void broadcast(const std::string& text);
void private_msg(uint32_t client_id, const std::string& text);
void kick(uint32_t client_id, const std::string& reason = "");
void send_file_to_client(uint32_t client_id, const std::string& path);
void send_file_to_all(const std::string& path);
bool poll(GuiServerEvent& ev);
std::vector<GuiServerClientInfo> client_list() const;
size_t client_count() const;
```

**Thread safety contract**
- `GuiClient::connect()` and `GuiServer::start()` are called from the **main thread**.
- The `Client` receive thread calls `push_event()` (private), which is mutex-protected.
- `poll()` is called from the **main thread** and drains one event per call.
- Never call ImGui functions from the receive thread.

**Connect button logic**

```cpp
if (button_pressed) {
    g_config.host         = app.cs_host;
    g_config.port         = app.cs_port;
    g_config.nickname     = app.cs_nick;
    g_config.encrypt_type = static_cast<EncryptType>(app.cs_encrypt);
    g_config.encrypt_key  = (app.cs_encrypt != 0) ? app.cs_key : "";

    if (!net_init()) {
        app.set_status("Failed to initialise networking.", true);
    } else if (app.gui_client.connect(app.cs_host, app.cs_port)) {
        app.screen = AppScreen::Client;
        app.set_status("Connected to " + std::string(app.cs_host) +
                       ":" + std::to_string(app.cs_port));
    } else {
        app.set_status("Connection failed. Is the server running?", true);
    }
}
```

**Start Server button logic**

```cpp
if (button_pressed) {
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
```

**Done when**
- [ ] 🔴 Connecting to a running `safesocket server` process changes the screen to Client.
- [ ] 🔴 Starting the server changes the screen to Server.
- [ ] 🔴 Connecting with the wrong port shows a red status banner.
- [ ] 🔴 `net_cleanup()` is called on window close.
- [ ] 🟡 Closing the GUI while connected does not crash.
- [ ] 🟡 Starting the server twice without stopping shows an error.

**Commit message:** `gui: task 4 — networking connect/disconnect`

---

## Task 5 — Chat Panel: Broadcast & Peer List

**Goal:** Implement the full chat window so that messages can be sent and received
in real time, and the peer/client list is kept live.

**Implements CLI equivalents:**
- Client: plain message (broadcast), `/broadcast <text>`, `/list`, `/myid`
- Server: `/broadcast <text>`, `/list`

**Files to touch**

| File | Action |
|---|---|
| `gui/src/window_chat.cpp` | Full implementation |
| `gui/src/app.cpp` | `app_poll_events()` — drain `GuiClientEvent::Message` and `PeerListUpdate`; drain `GuiServerEvent::Message` and `ClientConnected/Disconnected` |

**Layout specification**

```
┌──────────────────┬────────────────────────────────────────┐
│  Online Peers    │  [08:42:11] Alice: hello world          │
│  ──────────────  │  [08:42:14] * Bob joined               │
│  [2] Alice (you) │  [08:42:18] Bob: hey everyone          │
│  [3] Bob         │                                        │
│  [4] Charlie     │  [08:42:22] Server: notice             │
│                  │  ──────────────────────────────────────│
│                  │  [ type a message...       ] [ Send ]  │
└──────────────────┴────────────────────────────────────────┘
  ← 180 px →       ← fills remaining width →
```

**Colour rules for message rows**
| Type | Sender colour | Text |
|---|---|---|
| Broadcast | Cyan `(0.4, 0.75, 1.0, 1.0)` | Normal text colour |
| Private (PM) | Amber `(1.0, 0.75, 0.3, 1.0)` | Normal text colour |
| System / join / kick | Yellow `(0.7, 0.7, 0.4, 1.0)` | Italic prefix `*` |
| Own name in peer list | Green `(0.4, 0.9, 0.4, 1.0)` | Appended `(you)` |

**Auto-scroll:** `ImGui::SetScrollHereY(1.0f)` when the scroll position is near the bottom (within 10 px of `GetScrollMaxY()`).

**Send logic**
- Input: `ImGuiInputTextFlags_EnterReturnsTrue` — pressing Enter triggers send.
- After send, clear the buffer and call `ImGui::SetKeyboardFocusHere(-1)` to
  return focus to the input.
- In **client mode**: call `app.gui_client.send_broadcast(text)`.
- In **server mode**: call `app.gui_server.broadcast(text)` and push a local
  `ChatMessage` immediately (the server does not echo back to itself).

**Event draining in `app_poll_events()`** (client side)

```cpp
GuiClientEvent ev;
while (app.gui_client.poll(ev)) {
    switch (ev.type) {
    case GuiClientEvent::Type::Message:
        app.push_message(ev.sender, ev.text, ev.is_private, false);
        break;
    case GuiClientEvent::Type::SystemMsg:
        app.push_message("*", ev.text, false, true);
        break;
    case GuiClientEvent::Type::PeerListUpdate:
        break; // peers() is read directly each frame
    case GuiClientEvent::Type::Disconnected:
        app.screen = AppScreen::Connect;
        app.set_status("Disconnected from server.", true);
        break;
    // ... (file events handled in Task 7)
    }
}
```

**Done when**
- [ ] 🔴 Sending a message in Client mode broadcasts it; other connected clients see it.
- [ ] 🔴 Incoming messages appear with the correct sender name and timestamp.
- [ ] 🔴 Peer list updates when clients join or leave.
- [ ] 🔴 The history scrolls automatically as messages arrive.
- [ ] 🔴 Own entry in the peer list is tinted green with `(you)`.
- [ ] 🔴 Server-mode broadcast appears in the server's own chat log.
- [ ] 🟡 Long messages wrap; they do not overflow the panel.
- [ ] 🟡 An unexpected disconnect switches back to the Connect screen with a red banner.

**Commit message:** `gui: task 5 — chat panel, broadcast, peer list`

---

## Task 6 — Private Messaging & Nick Change

**Goal:** Add right-click context menu on peers (client mode) and the selected-row
PM field in server mode, and implement live nickname changing.

**Implements CLI equivalents:**
- Client: `/msg <id> <text>`, `/msgn <nick> <text>`, `/nick <newname>`
- Server: `/msg <id> <text>`, `/msgn <nick> <text>`

**Files to touch**

| File | Action |
|---|---|
| `gui/src/window_chat.cpp` | Peer list: right-click context menu + nick-change input |
| `gui/src/window_server.cpp` | Client table: PM input row beneath the table |

**Client-mode peer right-click menu**

```cpp
if (ImGui::BeginPopupContextItem()) {
    ImGui::Text("Actions for %s [%u]", peer.nickname.c_str(), peer.id);
    ImGui::Separator();
    if (ImGui::MenuItem("Send private message")) { /* open PM input */ }
    if (ImGui::MenuItem("Send file"))            { /* open file path input */ }
    ImGui::EndPopup();
}
```

When "Send private message" is selected, display a small input line below the
peer list (or as a modal) with: `[input box for text]  [Send PM]`.
On send: call `app.gui_client.send_private(peer.id, text)` and push a local
`ChatMessage` with `is_private = true`, sender = `"You → <nick>"`.

**Nick-change widget** — add a small section at the bottom of the peer list panel:

```
──────────────────
Nick: [ alice   ]  [Change]
```

On press (or Enter): call `app.gui_client.send_nick_change(new_nick)`, update
`g_config.nickname`, and show a green status `"Nickname changed to <new_nick>"`.

**Server-mode PM** — below the client table (already laid out in the scaffold):

```
Selected: [3]   [ type private message ... ]  [PM]   [ file path ... ]  [Send File]
```

On PM send: `app.gui_server.private_msg(selected_id, text)` and push a local message.

**Done when**
- [ ] 🔴 Client can send a PM to any peer; the PM appears in the chat log in amber.
- [ ] 🔴 Server can send a PM to any connected client.
- [ ] 🔴 Nick change is reflected in the peer list on the next `MSG_CLIENT_LIST` update.
- [ ] 🟡 The PM input field clears after sending.
- [ ] 🟡 Right-clicking your own entry in the peer list shows no "Send private message" option.

**Commit message:** `gui: task 6 — private messaging and nick change`

---

## Task 7 — File Transfer

**Goal:** Implement file send (client→client, client→server, server→client,
server→all) and the file transfer progress panel.

**Implements CLI equivalents:**
- Client: `/sendfile <id> <path>`, `/sendfileserver <path>`
- Server: `/sendfile <id> <path>`, `/sendfileall <path>`

**Files to touch**

| File | Action |
|---|---|
| `gui/src/window_files.cpp` | Full implementation (progress bars, reject/done states) |
| `gui/src/window_chat.cpp` | Add "Send file" option to peer right-click menu |
| `gui/src/window_server.cpp` | Add "Send File" and "Send to All" buttons |
| `gui/src/app.cpp` | Drain `FileRequest`, `FileProgress`, `FileComplete`, `FileRejected` events |

**Files panel layout**

```
┌────────────────────────────────────────────────────────────┐
│  File Transfers                                            │
│  ──────────────────────────────────────────────────────── │
│  ↑  report.pdf   (Bob)                                    │
│  ████████████████░░░░  78%  (4.6 MB / 5.9 MB)            │
│                                                            │
│  ↓  photo.jpg    (Alice)                                   │
│  ████████████████████  Done — 2.1 MB              [green] │
│                                                            │
│  ↑  archive.zip  (Server)                                  │
│  ░░░░░░░░░░░░░░░░░░░░  Rejected                   [red]   │
└────────────────────────────────────────────────────────────┘
```

**Progress bar colour coding**
| State | `ImGuiCol_PlotHistogram` |
|---|---|
| In progress | `(0.2, 0.4, 0.8, 1.0)` — blue |
| Complete | `(0.2, 0.7, 0.2, 1.0)` — green |
| Rejected | `(0.7, 0.2, 0.2, 1.0)` — red |

**Direction indicator:** `↑` in green tint for uploads, `↓` in blue tint for downloads.

**Send file from client right-click menu**

```cpp
if (ImGui::MenuItem("Send file")) {
    // Store selected peer ID; show path input on next frame
    app.pending_file_peer_id = peer.id;
    app.show_file_send_input = true;
}
```

When `show_file_send_input` is true, render a small input at the bottom of
the chat panel: `[ file path ]  [Send]`. On send, call
`app.gui_client.send_file(peer_id, path)`, push a `FileTransferEntry`
with `is_upload = true`.

**Auto-accept** — if `g_config.auto_accept_files == true`, incoming file requests
from the `GuiClientEvent::FileRequest` event are accepted automatically.
If false (default), push a system message: `"Incoming file: <name> from <peer> — auto-accepting"` or
`"Incoming file: <name> from <peer> — [Accept] [Reject]"` (buttons in the system message row
are acceptable in v0.0.2; a modal is planned for v0.0.3).

**Note on progress accuracy:** In v0.0.2 progress bars reflect chunk count,
not actual bytes, because `MSG_FILE_PROGRESS` does not exist until v0.0.4.
Use the `FileTransferEntry::bytes_done` field and set it to `chunk_index * FILE_CHUNK`
(65 536 bytes per chunk as defined in `protocol.hpp`).

**Done when**
- [ ] 🔴 Client can send a file to another client; the entry appears in the Files tab with a progress bar.
- [ ] 🔴 Client can upload a file to the server.
- [ ] 🔴 Server can send a file to a specific client.
- [ ] 🔴 Server can send a file to all clients simultaneously.
- [ ] 🔴 Completed transfers show green bar with "Done — X MB".
- [ ] 🔴 Rejected transfers show red bar with "Rejected".
- [ ] 🟡 Transfers persist in the panel after completion until the GUI is restarted.
- [ ] 🟡 "No active or recent transfers" placeholder shown when the list is empty.

**Commit message:** `gui: task 7 — file transfer panel and send operations`

---

## Task 8 — Settings Panel & Config Persistence

**Goal:** Replace the settings placeholder with a live editor for every `Config`
field. Changes take effect immediately (same as CLI `/set`). Save and Load buttons
write to and read from `safesocket.conf`.

**Implements CLI equivalents:** `/config`, `/set <key> <value>`, `/saveconfig`,
`/loadconfig`, `genconfig`, `--config <file>` flag.

**Files to touch**

| File | Action |
|---|---|
| `gui/src/window_settings.cpp` | Full implementation |
| `gui/src/main_gui.cpp` | Handle `--config` flag at startup |

**Settings panel sections**

Each section has a coloured section header `(0.80, 0.80, 0.40, 1.0)` — yellow.

| Section | Config fields shown |
|---|---|
| Network | `host`, `port`, `max_clients`, `recv_timeout`, `connect_timeout` |
| Identity | `nickname`, `server_name`, `motd` |
| Encryption | `encrypt_type` (combo), `encrypt_key` (password), `require_key`, `access_key` (password) |
| File Transfer | `download_dir`, `auto_accept_files`, `max_file_size` |
| Logging | `log_to_file`, `log_file`, `verbose` |
| Keepalive | `keepalive`, `ping_interval` |
| Misc | `color_output`, `buffer_size` |

**Important implementation notes**
- Fields that are network-affecting (`host`, `port`) should show a tooltip:
  `"Restart required — changing this does not affect an active connection"`.
- `encrypt_key` and `access_key` use `ImGuiInputTextFlags_Password`.
- The `encrypt_type` field uses the same 4-item combo as the connect screen.
- All fields call `ImGui::InputText`/`InputInt`/`Checkbox` and write the new
  value directly to `g_config` on change.

**Bottom button row**

```
[ Save to safesocket.conf ]   [ Load safesocket.conf ]   [ Generate default config ]
```

- Save: calls `g_config.save("safesocket.conf")` → green status "Config saved".
- Load: calls `g_config.load("safesocket.conf")` → green status "Config loaded",
  or red status on failure.
- Generate default: calls `g_config.save("safesocket.conf")` with default values
  (same as CLI `genconfig`).

**`--config` flag handling in `main_gui.cpp`**

```cpp
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
```

**Done when**
- [ ] 🔴 All 24 config fields are editable in the Settings panel.
- [ ] 🔴 Changes to `nickname` are reflected in `g_config` immediately.
- [ ] 🔴 Save writes `safesocket.conf`; reopening the GUI and loading restores all values.
- [ ] 🔴 `--config myfile.conf` pre-fills the connect screen with the loaded values.
- [ ] 🟡 Network-affecting fields show a "Restart required" tooltip.
- [ ] 🟡 Password fields are masked by default.
- [ ] 🟢 A small `[?]` help icon next to each field shows the same description as `confighelp` in the CLI.

**Commit message:** `gui: task 8 — settings panel and config persistence`

---
---

# ✅ PARITY CHECKPOINT

After Task 8 is complete, the GUI supports **every capability of the CLI v0.0.1 binary**:

| CLI capability | GUI equivalent |
|---|---|
| `safesocket server` | Start Server button |
| `safesocket client --nick … --host … --port …` | Connect as Client |
| Broadcast message | Chat input + Send |
| Private message `/msg` `/msgn` | Peer right-click → PM / Server table PM row |
| `/list` | Peer list panel (live, updates automatically) |
| `/myid` | Own entry highlighted green with `(you)` |
| `/nick <n>` | Nick field at bottom of peer panel |
| `/sendfile <id> <path>` | Peer right-click → Send file |
| `/sendfileserver <path>` | Server entry in peer list → Send file |
| `/sendfile` / `/sendfileall` (server) | Server Clients tab Send File / Send to All |
| `/config` `/set` `/saveconfig` `/loadconfig` | Settings tab |
| `genconfig` | "Generate default config" button in Settings |
| `/quit` | Window close (with `net_cleanup()`) |
| `/stats` (server) | Displayed in server Clients tab header row |
| `/kick` (server) | Kick button per row in client table |

---
---

# PHASE 2 — Future Version GUI Additions

Each block below tracks the GUI work required for that version.
Start these only after the version's CLI changes are merged and stable.

---

## v0.0.2 GUI — Bug Fix Visibility

**CLI changes in this version:** BUG-01 through BUG-04 (race conditions, error messages, Config::set hardening).

**GUI tasks**

- [ ] `window_connect.cpp` — connect failure now shows the full improved error message from `[net]` (BUG-01/04 fix). Surface it in the status banner rather than a generic "Connection failed".
- [ ] `window_settings.cpp` — `Config::set()` no longer throws; remove any `try/catch` blocks added around settings writes.
- [ ] `app.cpp` — `GuiClientEvent::Disconnected` handler: use the new actionable disconnect reason string from `MSG_ERROR` rather than a hardcoded string.

**Done when**
- [ ] 🟡 Connecting to an unreachable host shows "Connection refused. Is the server running?" in the status banner.
- [ ] 🟡 Connecting with a wrong access key shows "Access denied: wrong or missing key".
- [ ] 🟡 No crashes when entering a non-numeric value in any Settings int field.

**Commit message:** `gui: v0.0.2 — surface improved error messages`

---

## v0.0.3 GUI — Observability

**CLI changes in this version:** Log levels, JSON log format, log rotation, `/status` command, extended `/stats`.

**GUI tasks**

- [ ] **Settings panel — new section "Logging v2":** Add fields for `log_level` (combo: ERROR/WARN/INFO/DEBUG), `log_format` (combo: text/json), `log_max_size_mb`, `log_max_files`.
- [ ] **Server tab — stats bar:** Replace the plain "Connected: N / 64" line with the full `/status` output: `uptime · clients · msgs · tx · rx` in a single formatted line.
- [ ] **Server tab — `/stats` modal:** "Show Stats" button opens an `ImGui::BeginPopupModal` with the full counter table (messages_sent, messages_received, bytes_sent, bytes_received, files_transferred, auth_failures, uptime_seconds).
- [ ] **Light theme option:** Add a `bool use_light_theme` to `AppState`. Toggle button in Settings → Misc section calls either `apply_dark_theme()` or `apply_light_theme()` (implement both).

**Done when**
- [ ] 🟡 All 4 new log config keys are present in Settings.
- [ ] 🟡 Server stats modal opens and shows live counters.
- [ ] 🟡 Switching theme in Settings changes the appearance immediately without restart.

**Commit message:** `gui: v0.0.3 — observability panel, stats modal, theme toggle`

---

## v0.0.4 GUI — Expansion

**CLI changes in this version:** `MSG_FILE_PROGRESS`, named rooms, reconnection, raised client ceiling (256).

**GUI tasks**

- [ ] **Files panel — byte-accurate progress:** `GuiClientEvent::FileProgress` now carries real `bytes` from `MSG_FILE_PROGRESS`. Update `FileTransferEntry::bytes_done` and `total_bytes` from this event. Remove the chunk-count estimation from Task 7.
- [ ] **Files panel — transfer speed:** Track timestamps per `FileTransferEntry` and compute bytes/sec over a 1-second rolling window. Display as `@ 1.2 MB/s` next to the progress bar overlay.
- [ ] **Rooms panel (new tab):** Add a "Rooms" tab in both Client and Server tab bars.
  - Left column: list of active rooms, with member count.
  - Buttons: `[Join]`, `[Leave]`, `[Create]`.
  - Entering a room switches the chat panel to show only that room's messages.
  - Default room `#general` is pre-joined; shown selected at startup.
- [ ] **Reconnect indicator:** When `g_config.reconnect == true` and the connection drops, show a spinner/banner "Reconnecting… attempt N / max" instead of immediately returning to the Connect screen.
- [ ] **Settings — new reconnect section:** `reconnect` (checkbox), `reconnect_max_attempts`, `reconnect_delay_ms`.

**Done when**
- [ ] 🔴 Progress bars are byte-accurate (not chunk-count-estimated).
- [ ] 🔴 Transfer speed is displayed and updates live.
- [ ] 🔴 Rooms tab allows joining and leaving rooms; room-scoped messages work.
- [ ] 🟡 Reconnect spinner appears on unexpected disconnect when reconnect is enabled.

**Commit message:** `gui: v0.0.4 — byte-accurate progress, speed display, rooms tab, reconnect`

---

## v0.0.5 GUI — Security

**CLI changes in this version:** ChaCha20-Poly1305 cipher, X25519 key exchange, legacy cipher warnings.

**GUI tasks**

- [ ] **Connect screen — cipher combo:** Add `"CHACHA20"` as a 5th item. Mark `"XOR"`, `"VIGENERE"`, `"RC4"` with a `⚠` prefix in the combo to visually flag them as legacy.
- [ ] **Connect screen — legacy cipher warning banner:** When an insecure cipher is selected, show an inline yellow warning below the combo: `"⚠ XOR/VIGENERE/RC4 provide obfuscation only. Use CHACHA20 for real security."`.
- [ ] **Settings — encryption section:** Same combo update. Warning banner shows when a legacy cipher is the active `g_config.encrypt_type`.
- [ ] **Handshake indicator:** During the X25519 DH handshake (between `connect()` and the first `MSG_CLIENT_LIST`), show a small "🔒 Establishing secure session…" label in the status bar. Replace with "🔒 Session secured (ChaCha20)" once complete.

**Done when**
- [ ] 🔴 CHACHA20 is selectable in both the connect screen and settings.
- [ ] 🔴 Selecting a legacy cipher shows the inline warning.
- [ ] 🟡 Handshake indicator appears and disappears correctly.

**Commit message:** `gui: v0.0.5 — ChaCha20 cipher option, legacy cipher warnings, handshake indicator`

---

## v0.0.6 GUI — Production

**CLI changes in this version:** `g_config` singleton removed, protocol v2, hardening, packaging.

**GUI tasks**

- [ ] **`g_config` singleton removal:** `GuiClient` and `GuiServer` adapters must pass `Config&` to `Client` and `Server` constructors rather than relying on the global. Update all adapter code accordingly.
- [ ] **Protocol v2 version field:** `MSG_CONNECT` now includes a version byte. If the server reports a version mismatch via `MSG_ERROR`, surface it as a red banner: "Server is running protocol v1 — please upgrade your client or server".
- [ ] **Nickname validation feedback:** The nick-change widget should validate client-side (printable ASCII, max 32 chars) and show a red tooltip inline before sending, rather than relying on the server to reject it.
- [ ] **App icon:** Set the SDL window icon using `SDL_SetWindowIcon()` with the `assets/safesocket_logo.png` loaded via SDL2's image loader (or a raw pixel conversion). Requires adding `SDL_image` as an optional dependency.
- [ ] **Build — packaging integration:** Ensure the CMake install target installs `safesocket-gui` alongside `safesocket` and the man page. Add a `CPACK` config for `.deb` and `.zip`.

**Done when**
- [ ] 🔴 `Config` is no longer a global in the GUI build — each adapter holds its own reference.
- [ ] 🔴 Protocol version mismatch is surfaced with a clear error banner.
- [ ] 🟡 Nick input validates client-side before sending.
- [ ] 🟢 Window icon is set on all three platforms.
- [ ] 🟢 `cmake --install` places `safesocket-gui` in the correct location.

**Commit message:** `gui: v0.0.6 — remove g_config dependency, protocol v2 handling, packaging`

---

## Commit Message Convention

```
gui: task N — short description
gui: v0.0.X — short description
```

Always prefix with `gui:` so GUI commits are easy to filter with `git log --grep="^gui:"`.
