# SafeSocket

**Cross-platform encrypted TCP socket chat & file transfer — C++11, no external dependencies.**

```
  ____         __      ____             __        __
 / __/__ _____/ /___  / __/__  _______/ /_____  / /_
/ _// _ `/ __/ / _ \ _\ \/ _ \/ __/  '_/ -_) / __/
/___/\_,_/_/ /_/\___//___/\___/\__/_/\_\\__/ \__/
  SafeSocket v1.0 - Encrypted P2P Chat & File Transfer
```

---

## Features

| Feature | Description |
|---------|-------------|
| **Real-time chat** | Broadcast to all clients or private messages |
| **Client ↔ Client** | Messages between clients routed via server |
| **File transfer** | Server → client, client → server, server → all clients |
| **Image/binary** | Any file type supported (binary-safe) |
| **Encryption** | NONE / XOR / VIGENERE / RC4 (no external libs) |
| **Access control** | Optional password to join server |
| **Keepalive** | Configurable ping/pong heartbeat |
| **Logging** | Optional log file output |
| **Config file** | Full INI-style configuration with live reload |
| **Runtime config** | Change settings without restarting |
| **Cross-platform** | Windows 10 (MinGW/Dev-C++) + Linux (GCC) |

---

## Building

### Linux (GCC)

```bash
# One command:
g++ -std=c++11 -O2 -pthread -o safesocket \
    main.cpp crypto.cpp config.cpp network.cpp server.cpp client.cpp

# Or use Makefile:
make
make debug   # debug build
```

### Windows 10 (Dev-C++ / MinGW)

**Option 1 — Batch script:**
```
build_windows.bat
```

**Option 2 — Dev-C++:**
1. Open `SafeSocket.dev` in Dev-C++
2. Press F9 (Build & Run) or Ctrl+F9 (Build)
3. Ensure compiler flags include: `-std=c++11 -lws2_32 -mthreads`

**Option 3 — Manual command:**
```bat
g++ -std=c++11 -O2 -o safesocket.exe ^
    main.cpp crypto.cpp config.cpp network.cpp server.cpp client.cpp ^
    -lws2_32 -mthreads
```

> **Requires:** TDM-GCC 64-bit or MinGW-w64 — https://jmeubank.github.io/tdm-gcc/

---

## Quick Start

### Start the server
```bash
./safesocket server
# Windows:
safesocket.exe server
```

### Connect clients (each in separate terminal)
```bash
./safesocket client --nick Alice
./safesocket client --nick Bob
```

### Encrypted example
```bash
# Server
./safesocket server --encrypt RC4 --key secretpassword

# Clients (must use same encryption + key)
./safesocket client --nick Alice --encrypt RC4 --key secretpassword
./safesocket client --nick Bob   --encrypt RC4 --key secretpassword
```

### With access key (password to join)
```bash
./safesocket server --require-key --access-key joinme123

./safesocket client --nick Alice --key joinme123
```

---

## Command-Line Arguments

```
safesocket server|client [options]

Options:
  --host <ip>            Server IP (default: 127.0.0.1)
  --port <port>          Port (default: 9000)
  --nick <name>          Nickname (client mode)
  --encrypt <type>       NONE | XOR | VIGENERE | RC4
  --key <passphrase>     Encryption or access key
  --config <file>        Config file path (default: safesocket.conf)
  --download-dir <dir>   Where received files are saved
  --log <file>           Log to file
  --verbose              Debug output
  --no-color             Disable ANSI color
  --max-clients <n>      Max simultaneous clients (server)
  --require-key          Require access key (server)
  --access-key <key>     Join password (server)
  --motd <text>          Message of the Day (server)
  --server-name <name>   Server display name
  --help                 Show help

safesocket genconfig [file]   Generate default config file
```

---

## Server Commands (CLI)

| Command | Description |
|---------|-------------|
| `/list` | List all connected clients with ID, nickname, IP |
| `/msg <id> <text>` | Private message to client by numeric ID |
| `/msgn <nick> <text>` | Private message to client by nickname |
| `/broadcast <text>` | Send to all connected clients |
| `/sendfile <id> <path>` | Send file to specific client |
| `/sendfileall <path>` | Send file to ALL clients simultaneously |
| `/kick <id> [reason]` | Disconnect a client |
| `/kickn <nick> [reason]` | Kick by nickname |
| `/stats` | Show bytes transferred, message count, etc. |
| `/config` | Display current configuration |
| `/set <key> <value>` | Change config value live (no restart needed) |
| `/saveconfig [file]` | Save current config to file |
| `/loadconfig [file]` | Reload config from file |
| `/confighelp` | List all configurable keys |
| `/quit` | Graceful shutdown |

---

## Client Commands (CLI)

| Command | Description |
|---------|-------------|
| `<message>` | Broadcast to all clients |
| `/broadcast <text>` | Explicit broadcast |
| `/msg <id> <text>` | Private message to client by ID |
| `/msgn <nick> <text>` | Private message by nickname |
| `/list` | Show all connected clients |
| `/myid` | Show your client ID |
| `/nick <newname>` | Change your nickname live |
| `/sendfile <id> <path>` | Send file to another client (via server) |
| `/sendfileserver <path>` | Upload file to server |
| `/config` | Show configuration |
| `/set <key> <value>` | Change config live |
| `/saveconfig [file]` | Save config |
| `/loadconfig [file]` | Reload config |
| `/confighelp` | List all config keys |
| `/quit` | Disconnect and exit |

---

## Configuration File

Generate with: `./safesocket genconfig`

```ini
# SafeSocket Configuration File

[network]
host             = 127.0.0.1
port             = 9000
max_clients      = 64
recv_timeout     = 300
connect_timeout  = 10

[identity]
nickname         = anonymous
server_name      = SafeSocket-Server
motd             = Welcome to SafeSocket!

[security]
encrypt_type     = NONE          # NONE | XOR | VIGENERE | RC4
encrypt_key      =               # Encryption passphrase
require_key      = false         # Require access key to join
access_key       =               # Join password

[transfer]
download_dir     = ./downloads   # Where received files are saved
auto_accept_files= false         # Auto-accept without prompting
max_file_size    = 0             # 0 = unlimited (bytes)

[logging]
log_to_file      = false
log_file         = safesocket.log
verbose          = false

[keepalive]
keepalive        = true
ping_interval    = 30            # Seconds between pings

[misc]
color_output     = true
buffer_size      = 4096
```

---

## Encryption Details

| Type | Description | Strength |
|------|-------------|----------|
| `NONE` | No encryption, plaintext | None |
| `XOR` | XOR each byte with repeating key | Weak (educational) |
| `VIGENERE` | Byte-level Vigenère cipher | Weak (educational) |
| `RC4` | RC4 stream cipher, industry-standard | Strong |

> **Recommended:** Use `RC4` with a long, random passphrase for real security.
> All encryption operates on the packet payload after the header.
> Server and all clients must use the same `encrypt_type` and `encrypt_key`.

---

## Packet Protocol

All communication uses a 20-byte binary header:

```
[4 bytes] Magic     = 0x534B5A4F "OKZS"
[4 bytes] Msg Type  (see MsgType enum in protocol.hpp)
[4 bytes] Sender ID (0 = server, 0xFFFFFFFF = broadcast)
[4 bytes] Target ID
[4 bytes] Data length
[N bytes] Payload   (optionally encrypted)
```

Message flow:
- **Client → Server → All clients**: MSG_BROADCAST
- **Client → Server → Target client**: MSG_PRIVATE (target_id set)
- **Server → Client**: MSG_PRIVATE / MSG_BROADCAST
- **File transfer**: MSG_FILE_START → MSG_FILE_ACCEPT/REJECT → N×MSG_FILE_DATA → MSG_FILE_END

---

## Notes

- **Localhost default**: Server binds to `127.0.0.1:9000`. For LAN use, set `--host 0.0.0.0`.
- **File transfer** is synchronous per-connection (server pauses other messages to that client while transferring).
- **Client-to-client** messaging is always routed through the server.
- **Downloads** go to `./downloads/` by default (auto-created).
- **Dev-C++ version**: Tested with TDM-GCC 4.9.2 64-bit. Ensure C++11 is enabled in compiler settings.
