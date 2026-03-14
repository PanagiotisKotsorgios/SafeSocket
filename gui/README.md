# SafeSocket GUI — v0.0.1

This directory contains the **optional graphical front-end** for SafeSocket, built on top of
[Dear ImGui](https://github.com/ocornut/imgui) and [SDL2](https://www.libsdl.org/).

The CLI binary (`safesocket`) is **unchanged and fully independent**. The GUI is a
separate binary (`safesocket-gui`) that links against the same core source files:
`crypto.cpp`, `config.cpp`, `network.cpp`, `server.cpp`, `client.cpp`.

---

## Why a Separate Binary?

| Approach | Description | Verdict |
|---|---|---
| `--gui` flag (single binary) | One binary, mode chosen at runtime | ❌ Forces every deployment to carry SDL2/ImGui, even headless servers |
| Separate `safesocket-gui` binary | Independent build, independent deploy | ✅ **Chosen** — zero bloat on servers, full GUI on desktops |

A server operator running on a headless VPS never needs to install SDL2.
A desktop user can install only the GUI binary and ignore the CLI entirely.
Both share the exact same protocol, config format, and core logic.

---

## Architecture

```
safesocket-gui
│
├── gui/src/main_gui.cpp          Entry point — SDL2 + ImGui init, main loop
├── gui/src/app.cpp               Application state machine, window routing
├── gui/src/window_connect.cpp    Connection/login screen
├── gui/src/window_chat.cpp       Main chat panel (messages, input, peers)
├── gui/src/window_server.cpp     Server control panel (client list, commands)
├── gui/src/window_files.cpp      File transfer queue and progress bars
├── gui/src/window_settings.cpp   Settings editor (maps to Config fields)
├── gui/src/gui_client.cpp        Thin adapter — wraps Client for GUI use
├── gui/src/gui_server.cpp        Thin adapter — wraps Server for GUI use
│
├── gui/include/app.hpp
├── gui/include/gui_client.hpp
├── gui/include/gui_server.hpp
│
├── gui/third_party/imgui/        Dear ImGui source (vendored, see below)
│   ├── imgui.h / imgui.cpp
│   ├── imgui_draw.cpp
│   ├── imgui_tables.cpp
│   ├── imgui_widgets.cpp
│   └── backends/
│       ├── imgui_impl_sdl2.h / .cpp
│       └── imgui_impl_opengl3.h / .cpp
│
└── gui/fonts/                    Optional custom fonts (.ttf)
    └── README.md
```

The `src/` core files are **shared** — the GUI binary and CLI binary compile them identically.
No changes to `server.cpp`, `client.cpp`, or the protocol are needed for v0.0.2.

---

## Dependencies

| Library | Version | Purpose | Install |
|---|---|---|---|
| Dear ImGui | 1.90.x | Immediate-mode GUI | Vendored in `third_party/imgui/` |
| SDL2 | 2.28+ | Window, input, OpenGL context | System package |
| OpenGL | 3.3+ | ImGui renderer backend | Provided by OS/GPU driver |

### Installing SDL2

**Ubuntu / Debian**
```bash
sudo apt install libsdl2-dev
```

**macOS (Homebrew)**
```bash
brew install sdl2
```

**Windows (vcpkg)**
```bat
vcpkg install sdl2:x64-windows
```

**Windows (manual)** — download the SDL2 development libraries from https://libsdl.org/download-2.0.php
and point the compiler at the `include/` and `lib/` directories.

---

## Getting Dear ImGui (third_party setup)

ImGui is **not** bundled in the repository to keep the repo size small.
Run the bootstrap script once after cloning:

```bash
# From the repository root
cd gui
./scripts/fetch_imgui.sh        # Linux / macOS
scripts\fetch_imgui.bat         # Windows
```

The script clones the `docking` branch of Dear ImGui into `third_party/imgui/`
and copies only the files listed in `third_party/imgui/FILES.txt`.

Alternatively, manually copy these files into `gui/third_party/imgui/`:

```
imgui.h             imgui.cpp
imgui_internal.h    imconfig.h          imstb_rectpack.h
imgui_draw.cpp      imgui_tables.cpp    imgui_widgets.cpp
backends/imgui_impl_sdl2.h    backends/imgui_impl_sdl2.cpp
backends/imgui_impl_opengl3.h backends/imgui_impl_opengl3.cpp
```

---

## Building

### Linux / macOS

```bash
cd gui
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
# Output: build/safesocket-gui
```

#### Manual (without CMake)

```bash
SDL2_FLAGS=$(sdl2-config --cflags --libs)

g++ -std=c++11 -O2 -pthread $SDL2_FLAGS \
    -I../include \
    -I../third_party/imgui \
    -I../third_party/imgui/backends \
    -I../../src \
    ../src/main_gui.cpp \
    ../src/app.cpp \
    ../src/window_connect.cpp \
    ../src/window_chat.cpp \
    ../src/window_server.cpp \
    ../src/window_files.cpp \
    ../src/window_settings.cpp \
    ../src/gui_client.cpp \
    ../src/gui_server.cpp \
    ../third_party/imgui/imgui.cpp \
    ../third_party/imgui/imgui_draw.cpp \
    ../third_party/imgui/imgui_tables.cpp \
    ../third_party/imgui/imgui_widgets.cpp \
    ../third_party/imgui/backends/imgui_impl_sdl2.cpp \
    ../third_party/imgui/backends/imgui_impl_opengl3.cpp \
    ../../src/crypto.cpp \
    ../../src/config.cpp \
    ../../src/network.cpp \
    ../../src/server.cpp \
    ../../src/client.cpp \
    -lGL \
    -o safesocket-gui
```

### Windows — MinGW / TDM-GCC

```bat
set SDL2_DIR=C:\SDL2-2.28.5-mingw
set IMGUI_DIR=..\third_party\imgui

g++ -std=c++11 -O2 ^
    -I%SDL2_DIR%\include -I%IMGUI_DIR% -I%IMGUI_DIR%\backends -I..\..\src ^
    ..\src\main_gui.cpp ..\src\app.cpp ..\src\window_connect.cpp ^
    ..\src\window_chat.cpp ..\src\window_server.cpp ..\src\window_files.cpp ^
    ..\src\window_settings.cpp ..\src\gui_client.cpp ..\src\gui_server.cpp ^
    %IMGUI_DIR%\imgui.cpp %IMGUI_DIR%\imgui_draw.cpp ^
    %IMGUI_DIR%\imgui_tables.cpp %IMGUI_DIR%\imgui_widgets.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_sdl2.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_opengl3.cpp ^
    ..\..\src\crypto.cpp ..\..\src\config.cpp ..\..\src\network.cpp ^
    ..\..\src\server.cpp ..\..\src\client.cpp ^
    -L%SDL2_DIR%\lib\x64 -lSDL2 -lSDL2main -lopengl32 ^
    -lws2_32 -mthreads -mwindows ^
    -o safesocket-gui.exe
```

### Windows — MSVC

```bat
cl /std:c++17 /EHsc /O2 ^
   /I%SDL2_DIR%\include /I%IMGUI_DIR% /I%IMGUI_DIR%\backends /I..\..\src ^
   main_gui.cpp app.cpp window_connect.cpp window_chat.cpp window_server.cpp ^
   window_files.cpp window_settings.cpp gui_client.cpp gui_server.cpp ^
   imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp ^
   imgui_impl_sdl2.cpp imgui_impl_opengl3.cpp ^
   ..\..\src\crypto.cpp ..\..\src\config.cpp ..\..\src\network.cpp ^
   ..\..\src\server.cpp ..\..\src\client.cpp ^
   /Fe:safesocket-gui.exe ^
   /link SDL2.lib SDL2main.lib opengl32.lib ws2_32.lib
```

---

## Running

```bash
./safesocket-gui
```

No command-line arguments are required. All settings are configured through the GUI.
A config file (`safesocket.conf`) is read from the working directory on startup if present.

### Optional flags (same as CLI)

```
--config <file>      Load config from this path instead of safesocket.conf
--no-color           Has no effect in GUI mode (accepted silently for scripting compatibility)
```

---

## GUI Windows

### Connect Screen
Shown at startup. Fill in host, port, nickname, encryption, and key, then press
**Connect as Client** or **Start Server**.

### Chat Panel
- Left column: peer list with IDs and nicknames
- Centre: message history with timestamps and sender labels
- Bottom: text input + **Send** button
- Right-click a peer → **Private message** / **Send file**

### Server Panel *(visible only in server mode)*
- Connected client table: ID, nickname, IP, uptime
- Buttons: Kick, Private message, Send file to selected
- Broadcast text input
- Live stats: uptime, messages, bytes transferred

### File Transfer Panel
- Active and completed transfers shown as progress bars
- Each row: filename, direction (↑ / ↓), peer, size, speed, status
- Cancel button for in-progress transfers

### Settings Panel
- All `Config` fields editable live
- Changes take effect immediately (same semantics as CLI `/set`)
- **Save to file** / **Load from file** buttons

---

## Adding a New GUI Window

1. Create `gui/src/window_<name>.cpp` and `gui/include/window_<name>.hpp`
2. Add a `void draw_<name>_window(AppState& app)` function
3. Call it from the main loop in `app.cpp`
4. Register any new `AppState` fields needed in `app.hpp`

---

## Fonts

Drop any `.ttf` file into `gui/fonts/` and reference it in `main_gui.cpp`:

```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("../fonts/MyFont.ttf", 15.0f);
```

See `gui/fonts/README.md` for recommended free fonts.

---

## Known Limitations (v0.0.2)

- File transfer progress bars reflect chunk count, not bytes, until `MSG_FILE_PROGRESS` lands in v0.0.4.
- The GUI client and server share the same `g_config` global as the CLI (tracked in technical debt; fixed in v0.0.6).
- SDL2 on Wayland may require `SDL_VIDEODRIVER=x11` as an env var on some distros.
- Dark theme only in v0.0.2; light theme planned for v0.0.3.

---

## License

Same as the root project: **MIT**. Dear ImGui is also MIT. SDL2 is zlib.
