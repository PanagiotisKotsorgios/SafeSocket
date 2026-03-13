<h1 align="center">The SafeSocket App</h1>

<br>

<p align="center">
  <img src="https://github.com/PanagiotisKotsorgios/SafeSocket/blob/main/assets/safesocket_logo.png" width="820" alt="SafeSocket Logo"/>
</p>



<p align="center">
  Cross-platform encrypted TCP chat and file transfer — C++11, zero external dependencies.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-0.0.1-blue" alt="Version"/>
  <img src="https://img.shields.io/badge/language-C%2B%2B11-lightgrey" alt="C++11"/>
  <img src="https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-informational" alt="Platform"/>
  <img src="https://img.shields.io/badge/dependencies-none-brightgreen" alt="Dependencies"/>
  <img src="https://img.shields.io/badge/license-MIT-green" alt="License"/>
</p>

---

<br>

| | |
|---|---|
| <img src="https://github.com/PanagiotisKotsorgios/SafeSocket/blob/main/assets/img1.jpg" width="400"/> | <img src="https://github.com/PanagiotisKotsorgios/SafeSocket/blob/main/assets/img2.jpg" width="400"/> |

<br>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Building](#building)
- [Quick Start](#quick-start)
- [Command-Line Reference](#command-line-reference)
- [Server Commands](#server-commands)
- [Client Commands](#client-commands)
- [Configuration File](#configuration-file)
- [Encryption](#encryption)
- [Wire Protocol](#wire-protocol)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

SafeSocket is a single-binary TCP application for real-time chat and file transfer between multiple clients. It runs identically on Linux, macOS, and Windows with no installation and no runtime dependencies — just compile and run.

A server instance accepts concurrent client connections. Clients can broadcast messages to everyone, send private messages, and transfer files of any type and size. All traffic is optionally encrypted at the packet level using one of three built-in cipher modes.

The project is written in strict C++11 and targets a zero-dependency build — every component including the network layer, encryption, and configuration parser is implemented from scratch.

---

## Features

| Feature | Detail |
|---|---|
| Real-time chat | Broadcast to all clients or send private messages by ID or nickname |
| Client-to-client routing | All messages routed through the server — no direct P2P socket required |
| File transfer | Server to client, client to client, server to all clients simultaneously |
| Binary-safe transfers | Any file type supported — images, archives, executables, binary data |
| Encryption | NONE / XOR / VIGENERE / RC4 — all implemented without external libraries |
| Access control | Optional server join password (`--require-key`) |
| Keepalive | Configurable ping/pong heartbeat with automatic dead-client detection |
| Logging | Optional log file with configurable path |
| Configuration file | INI-style config with comments, all keys documented |
| Live config reload | Change most settings at runtime without restarting |
| Cross-platform | Linux (GCC), macOS (Clang), Windows (MinGW / MSVC) |

---

## Building

### Requirements

| Platform | Compiler | Notes |
|---|---|---|
| Linux | GCC 4.8+ or Clang 3.4+ | `-pthread` required |
| macOS | Clang 3.4+ (Xcode CLT) | `-pthread` required |
| Windows | TDM-GCC 64-bit or MinGW-w64 | `-lws2_32 -mthreads` required |
| Windows | MSVC 2015+ | `/EHsc`, link `ws2_32.lib` |

### Linux / macOS

```bash
# Single command
g++ -std=c++11 -O2 -pthread -o safesocket \
    main.cpp crypto.cpp config.cpp network.cpp server.cpp client.cpp

# Debug build with sanitisers
g++ -std=c++11 -g -pthread -fsanitize=address,undefined \
    -o safesocket_dbg \
    main.cpp crypto.cpp config.cpp network.cpp server.cpp client.cpp

# Or use the Makefile
make          # release build
make debug    # debug build
make clean
```

### Windows — MinGW / TDM-GCC

```bat
REM Option 1: batch script
build_windows.bat

REM Option 2: manual command
g++ -std=c++11 -O2 -o safesocket.exe ^
    main.cpp crypto.cpp config.cpp network.cpp server.cpp client.cpp ^
    -lws2_32 -mthreads
```

> TDM-GCC 64-bit download: https://jmeubank.github.io/tdm-gcc/

### Windows — Dev-C++

1. Open `SafeSocket.dev` in Dev-C++
2. Verify compiler flags include: `-std=c++11 -lws2_32 -mthreads`
3. Press **F9** to build and run, or **Ctrl+F9** to build only

### Windows — MSVC

```bat
cl /std:c++17 /EHsc /W3 /O2 ^
   main.cpp crypto.cpp config.cpp network.cpp server.cpp client.cpp ^
   /Fe:safesocket.exe /link ws2_32.lib
```

### CMake (all platforms)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

---

## Quick Start

### Basic local chat

```bash
# Terminal 1 — start the server
./safesocket server

# Terminal 2 — first client
./safesocket client --nick Alice

# Terminal 3 — second client
./safesocket client --nick Bob
```

### Encrypted session

```bash
# Server — enable RC4 encryption with a passphrase
./safesocket server --encrypt RC4 --key my_passphrase

# Clients — must use the same cipher and key
./safesocket client --nick Alice --encrypt RC4 --key my_passphrase
./safesocket client --nick Bob   --encrypt RC4 --key my_passphrase
```

### Server with access control

```bash
# Only clients that supply the correct key are allowed in
./safesocket server --require-key --access-key joinme123

./safesocket client --nick Alice --key joinme123
```

### LAN usage

```bash
# Bind the server to all interfaces
./safesocket server --host 0.0.0.0 --port 9000

# Clients connect using the server's LAN IP
./safesocket client --nick Alice --host 192.168.1.50 --port 9000
```

### Generate a config file

```bash
./safesocket genconfig                  # writes safesocket.conf
./safesocket genconfig /etc/ss.conf     # custom path

# Start server using a config file
./safesocket server --config safesocket.conf
```

---

## Command-Line Reference

```
Usage:
  safesocket server  [options]
  safesocket client  [options]
  safesocket genconfig [path]

Options:
  --host <ip>            Bind address (server) or target address (client)
                         Default: 127.0.0.1
  --port <n>             TCP port. Default: 9000
  --nick <name>          Client display nickname. Default: anonymous
  --encrypt <type>       Cipher: NONE | XOR | VIGENERE | RC4
                         Default: NONE
  --key <passphrase>     Encryption passphrase or server access key
  --config <file>        Config file path. Default: safesocket.conf
  --download-dir <dir>   Directory for received files. Default: ./downloads
  --log <file>           Write log output to this file
  --verbose              Enable debug-level log output
  --no-color             Disable ANSI colour in CLI output
  --max-clients <n>      Maximum simultaneous clients (server). Default: 64
  --require-key          Require access key to connect (server)
  --access-key <key>     Access key clients must supply (server)
  --motd <text>          Message of the Day shown to connecting clients
  --server-name <name>   Server display name. Default: SafeSocket
  --help                 Print this help and exit
```

---

## Server Commands

Commands entered in the server terminal while the server is running.

| Command | Description |
|---|---|
| `/list` | List all connected clients — ID, nickname, IP address |
| `/msg <id> <text>` | Private message to a client by numeric ID |
| `/msgn <nick> <text>` | Private message to a client by nickname |
| `/broadcast <text>` | Send a message to all connected clients |
| `/sendfile <id> <path>` | Send a file to a specific client |
| `/sendfileall <path>` | Send a file to all connected clients simultaneously |
| `/kick <id> [reason]` | Disconnect a client by ID |
| `/kickn <nick> [reason]` | Disconnect a client by nickname |
| `/stats` | Show message count, bytes transferred, uptime |
| `/config` | Display the current configuration |
| `/set <key> <value>` | Change a config value at runtime without restarting |
| `/saveconfig [file]` | Save the current configuration to a file |
| `/loadconfig [file]` | Reload configuration from a file |
| `/confighelp` | List all valid configuration keys with descriptions |
| `/quit` | Graceful shutdown — disconnect all clients and exit |

---

## Client Commands

| Command | Description |
|---|---|
| `<message>` | Broadcast the message to all connected clients |
| `/broadcast <text>` | Explicit broadcast |
| `/msg <id> <text>` | Private message to a client by ID |
| `/msgn <nick> <text>` | Private message to a client by nickname |
| `/list` | Show all connected clients and their IDs |
| `/myid` | Show your own client ID |
| `/nick <newname>` | Change your display nickname live |
| `/sendfile <id> <path>` | Send a file to another client (routed via server) |
| `/sendfileserver <path>` | Upload a file to the server |
| `/config` | Show the current configuration |
| `/set <key> <value>` | Change a config value at runtime |
| `/saveconfig [file]` | Save the current configuration |
| `/loadconfig [file]` | Reload configuration from a file |
| `/confighelp` | List all valid configuration keys |
| `/quit` | Disconnect and exit |

---

## Configuration File

Generate a documented default configuration file:

```bash
./safesocket genconfig
```

```ini
# SafeSocket Configuration File
# Generated by: safesocket genconfig

# ── Network ──────────────────────────────────────────────────
host             = 127.0.0.1    # Bind/connect address
port             = 9000         # TCP port
max_clients      = 64           # Maximum simultaneous connections
recv_timeout     = 300          # Socket receive timeout (seconds)
connect_timeout  = 10           # Connect attempt timeout (seconds)

# ── Identity ─────────────────────────────────────────────────
nickname         = anonymous
server_name      = SafeSocket
motd             = Welcome to SafeSocket!

# ── Security ─────────────────────────────────────────────────
encrypt_type     = NONE         # NONE | XOR | VIGENERE | RC4
encrypt_key      =              # Encryption passphrase
require_key      = false        # Require access key to join
access_key       =              # Server join password

# ── File Transfer ─────────────────────────────────────────────
download_dir     = ./downloads  # Directory for received files
auto_accept_files= false        # Accept files without prompting
max_file_size    = 0            # Maximum file size in bytes (0 = unlimited)

# ── Logging ───────────────────────────────────────────────────
log_to_file      = false
log_file         = safesocket.log
verbose          = false

# ── Keepalive ─────────────────────────────────────────────────
keepalive        = true
ping_interval    = 30           # Seconds between pings

# ── Miscellaneous ─────────────────────────────────────────────
color_output     = true
buffer_size      = 4096
```

All keys can also be changed at runtime with `/set <key> <value>` without restarting the server or client.

---

## Encryption

All encryption operates on the packet payload after the 20-byte header. The header itself is always transmitted in plaintext to allow magic-byte validation. The server and all clients must use the same `encrypt_type` and `encrypt_key`.

| Mode | Algorithm | Strength | Use Case |
|---|---|---|---|
| `NONE` | Plaintext | None | Local development, trusted LAN |
| `XOR` | Repeating-key XOR | Obfuscation only | Educational use |
| `VIGENERE` | Byte-level Vigenère cipher | Obfuscation only | Educational use |
| `RC4` | RC4 stream cipher | Moderate | General use |

> **Note:** XOR and VIGENERE are intentionally weak and provided for educational purposes only.
> For any network environment where traffic may be observed, use RC4 with a long random passphrase.
> Proper authenticated encryption (ChaCha20-Poly1305) is planned for a future release — see [ROADMAP.md](ROADMAP.md).

---

## Wire Protocol

All communication uses a fixed 20-byte binary header followed by an optional payload.

```
Offset  Size  Field
──────  ────  ──────────────────────────────────────────────────
0       4     Magic      = 0x534B5A4F  ("OKZS" in ASCII)
4       4     Msg Type   (see MsgType enum in protocol.hpp)
8       4     Sender ID  (0 = server, 0xFFFFFFFF = broadcast)
12      4     Target ID
16      4     Data length (bytes of payload that follow)
20      N     Payload    (optionally encrypted)
```

### Message flow

| Flow | Message Types |
|---|---|
| Client → all clients | `MSG_BROADCAST` |
| Client → specific client | `MSG_PRIVATE` (target_id set) |
| Server → specific client | `MSG_PRIVATE` |
| File transfer | `MSG_FILE_START` → `MSG_FILE_ACCEPT` or `MSG_FILE_REJECT` → N × `MSG_FILE_DATA` → `MSG_FILE_END` |
| Keepalive | `MSG_PING` (server) → `MSG_PONG` (client) |

The full list of 18 message types is defined in `protocol.hpp`.

---

## Documentation

| Document | Description |
|---|---|
| [docs/usage.md](docs/usage.md) | Complete CLI flags, all config keys, all commands, examples |
| [docs/build.md](docs/build.md) | Detailed build instructions for all platforms and toolchains |
| [docs/architecture.md](docs/architecture.md) | Threading model, wire protocol, encryption pipeline, module map |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Coding standards, documentation style, PR process |
| [ROADMAP.md](ROADMAP.md) | Planned releases and long-range feature ideas |
| [SECURITY.md](SECURITY.md) | Vulnerability reporting policy and known limitations |
| [tests/README.md](tests/README.md) | Test suite structure, how to build and run tests |

---

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request — it covers coding standards, documentation requirements, naming conventions, and the PR checklist in detail.

For bug reports, open an issue with the version, platform, compiler, steps to reproduce, and expected vs. actual behaviour.
For security vulnerabilities, follow the process in [SECURITY.md](SECURITY.md) — do not open a public issue.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for the full text.
