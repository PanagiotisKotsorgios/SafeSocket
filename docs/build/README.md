
# Build Guide

SafeSocket is a self-contained C++11 application with **no external dependencies**. Everything — networking, encryption, configuration, and CLI — is implemented in the source tree. The only requirements are a C++11 compiler and the standard system socket library on each platform.

---

## Table of Contents

- [Requirements](#requirements)
- [Source Files](#source-files)
- [Building on Linux / macOS](#building-on-linux--macos)
  - [g++ / clang++ (manual)](#g--clang-manual)
  - [Makefile](#makefile)
  - [CMake](#cmake)
- [Building on Windows](#building-on-windows)
  - [MSVC (Visual Studio)](#msvc-visual-studio)
  - [MinGW / MSYS2 (g++ on Windows)](#mingw--msys2-g-on-windows)
  - [CMake + MSVC](#cmake--msvc)
- [Build Flags Reference](#build-flags-reference)
- [Compiler Warnings and Debug Builds](#compiler-warnings-and-debug-builds)
- [Verifying the Build](#verifying-the-build)
- [Troubleshooting](#troubleshooting)

---

## Requirements

| Platform | Compiler | Minimum Standard | System Library |
|----------|----------|-----------------|----------------|
| Linux | GCC ≥ 4.8 or Clang ≥ 3.4 | C++11 | pthread, BSD sockets (libc) |
| macOS | Apple Clang ≥ 5.0 | C++11 | pthread, BSD sockets (libc) |
| Windows | MSVC ≥ 2015, MinGW-w64 ≥ 5 | C++11 | `ws2_32.lib` (Winsock2) |

No third-party libraries. No package manager steps.

---

## Source Files

All source files live in the project root:

| File | Role |
|------|------|
| `main.cpp` | Entry point, argument parsing, mode dispatch |
| `server.cpp` / `server.hpp` | Multi-threaded TCP server |
| `client.cpp` / `client.hpp` | TCP client with receive thread |
| `network.cpp` / `network.hpp` | Cross-platform socket abstraction layer |
| `protocol.hpp` | Wire protocol: packet header, message types, payload helpers |
| `crypto.cpp` / `crypto.hpp` | In-place encryption: NONE, XOR, VIGENERE, RC4 |
| `config.cpp` / `config.hpp` | Runtime configuration, load/save/set/get |

---

## Building on Linux / macOS

### g++ / clang++ (manual)

Compile everything in one command:

```bash
# g++
g++ -std=c++11 -O2 -pthread \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp \
    -o safesocket

# clang++
clang++ -std=c++11 -O2 -pthread \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp \
    -o safesocket
```

Run it:

```bash
./safesocket --help
```

---

### Makefile

Create a `Makefile` in the project root:

```makefile
CXX      = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra -pthread
TARGET   = safesocket
SRCS     = main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
```

Build:

```bash
make
make clean   # remove object files and binary
```

To use clang instead:

```bash
make CXX=clang++
```

---

### CMake

Create `CMakeLists.txt` in the project root:

```cmake
cmake_minimum_required(VERSION 3.10)
project(SafeSocket CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(safesocket
    main.cpp
    server.cpp
    client.cpp
    network.cpp
    crypto.cpp
    config.cpp
)

# pthreads (Linux / macOS)
find_package(Threads REQUIRED)
target_link_libraries(safesocket PRIVATE Threads::Threads)

# Winsock (Windows)
if(WIN32)
    target_link_libraries(safesocket PRIVATE ws2_32)
endif()
```

Build with CMake:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Binary is at build/safesocket (Linux/macOS) or build/Release/safesocket.exe (Windows)
```

For a debug build:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

---

## Building on Windows

### MSVC (Visual Studio)

**Option A — Visual Studio IDE**

1. Open Visual Studio and choose **Create a new project → Empty Project (C++)**.
2. Add all `.cpp` files to the project: `main.cpp`, `server.cpp`, `client.cpp`, `network.cpp`, `crypto.cpp`, `config.cpp`.
3. Add all `.hpp` files so the IDE indexes them.
4. Open **Project → Properties → Configuration Properties**:
   - **C/C++ → Language**: set C++ Language Standard to **ISO C++11 Standard (/std:c++11)** or later.
   - **Linker → Input → Additional Dependencies**: add `ws2_32.lib`.
5. Press **Ctrl+Shift+B** to build.

**Option B — MSVC command line (Developer Command Prompt)**

```cmd
cl /EHsc /std:c++14 /O2 /Fe:safesocket.exe ^
   main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp ^
   ws2_32.lib
```

> Use the **"x64 Native Tools Command Prompt"** from the Visual Studio start menu folder to ensure `cl.exe` and `link.exe` are on the path.

---

### MinGW / MSYS2 (g++ on Windows)

Install MSYS2 from [https://www.msys2.org](https://www.msys2.org), then install the MinGW-w64 toolchain:

```bash
# In the MSYS2 MINGW64 shell
pacman -S mingw-w64-x86_64-gcc make
```

Build:

```bash
g++ -std=c++11 -O2 -pthread \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp \
    -o safesocket.exe \
    -lws2_32
```

Or use the Makefile from the Linux section — it works identically in the MSYS2 shell. Add `-lws2_32` to `CXXFLAGS` or as a separate `LDFLAGS` line:

```makefile
LDFLAGS = -lws2_32
```

---

### CMake + MSVC

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The `ws2_32` link is handled automatically by the `if(WIN32)` block in `CMakeLists.txt`.

For MinGW via CMake:

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

---

## Build Flags Reference

| Flag | Compiler | Purpose |
|------|----------|---------|
| `-std=c++11` | GCC / Clang | Enable C++11 (required minimum) |
| `/std:c++14` | MSVC | Enable C++14 (MSVC's closest equivalent) |
| `-O2` | GCC / Clang | Optimise for speed |
| `/O2` | MSVC | Optimise for speed |
| `-pthread` | GCC / Clang (Linux) | Link pthreads and enable thread-safe TLS |
| `-lws2_32` | MinGW | Link Winsock2 |
| `ws2_32.lib` | MSVC | Link Winsock2 |
| `-Wall -Wextra` | GCC / Clang | Enable extra warnings (recommended) |
| `/W4` | MSVC | Enable extra warnings |
| `-g` | GCC / Clang | Include debug symbols |
| `/Zi` | MSVC | Include debug symbols |
| `-DNDEBUG` | GCC / Clang | Disable assertions in release builds |
| `/DNDEBUG` | MSVC | Disable assertions in release builds |

---

## Compiler Warnings and Debug Builds

For development, build with full warnings and debug info:

```bash
# Linux / macOS — debug build
g++ -std=c++11 -g -O0 -Wall -Wextra -Wpedantic -pthread \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp \
    -o safesocket_debug
```

```bash
# Sanitisers (GCC/Clang, Linux only)
g++ -std=c++11 -g -O1 -pthread \
    -fsanitize=address,undefined \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp \
    -o safesocket_san
```

```cmd
REM Windows MSVC debug
cl /EHsc /std:c++14 /Zi /Od /W4 /Fe:safesocket_debug.exe ^
   main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp ^
   ws2_32.lib
```

---

## Verifying the Build

After a successful build, confirm the binary works:

```bash
# Linux / macOS
./safesocket --help

# Windows
safesocket.exe --help
```

Expected output starts with the SafeSocket ASCII banner followed by the usage text.

Quick loopback smoke test (requires two terminal windows):

```bash
# Terminal 1
./safesocket server --host 127.0.0.1 --port 9000 --verbose

# Terminal 2
./safesocket client --host 127.0.0.1 --port 9000 --nick TestUser
```

The server should print a connection notice, and the client should receive the MOTD.

---

## Troubleshooting

### Linux / macOS

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `undefined reference to pthread_create` | Missing `-pthread` | Add `-pthread` to both compile and link flags |
| `error: 'thread' is not a member of 'std'` | C++ standard too old | Use `-std=c++11` or newer |
| `Address already in use` on startup | Port still in TIME_WAIT | Wait ~30 s, or use a different port; `SO_REUSEADDR` is set by default |
| Permission denied on port < 1024 | Privileged port | Use a port ≥ 1024, or run with `sudo` (not recommended) |

### Windows

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `WSAEFAULT` / Winsock errors at runtime | Missing `WSAStartup` | Ensure `net_init()` is called before any socket operations; it is called automatically by `main.cpp` |
| `LNK2019: unresolved external symbol __WSAStartup` | Missing Winsock library | Add `ws2_32.lib` to linker input |
| ANSI colours show as raw escape codes | Old Windows console | Run in Windows Terminal, or add `--no-color` |
| `'cl' is not recognized` | Wrong command prompt | Use the **"Developer Command Prompt for VS"** shortcut |

### General

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Client connects but messages are garbled | Encryption mismatch | Ensure `--encrypt` and `--key` match on both server and client |
| Client rejected with `ACCESS DENIED` | Wrong or missing access key | Supply the correct `--key` matching the server's `--access-key` |
| File transfer stalls at 0 % | Receiver rejected the file | Check `auto_accept_files` or confirm the receiving user accepted |
