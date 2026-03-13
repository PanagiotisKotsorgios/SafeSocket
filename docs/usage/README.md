
# Usage Guide

This document covers every way to run SafeSocket - starting a server, connecting as a client, configuring the application at runtime, transferring files, and using the interactive CLI on both sides.

---

## Table of Contents

- [Invocation Syntax](#invocation-syntax)
- [Modes](#modes)
  - [server](#server-mode)
  - [client](#client-mode)
  - [genconfig](#genconfig-mode)
- [Command-Line Options](#command-line-options)
- [Configuration File](#configuration-file)
  - [All Configuration Keys](#all-configuration-keys)
  - [Runtime Changes with /set](#runtime-changes-with-set)
- [Encryption](#encryption)
- [Access Control](#access-control)
- [Server CLI Commands](#server-cli-commands)
- [Client CLI Commands](#client-cli-commands)
- [File Transfer](#file-transfer)
- [Quick-Start Examples](#quick-start-examples)

---

## Invocation Syntax

```
safesocket <mode> [options]
```

| Mode | Description |
|------|-------------|
| `server` | Start a multi-client TCP server |
| `client` | Connect to a server as a client |
| `genconfig` | Write a default config file and exit |

Run with no arguments, or with `--help`, to print usage:

```bash
./safesocket --help
```

---

## Modes

### Server Mode

Starts the server, binds to the configured address and port, and waits for connections.

```bash
# Minimal — bind on all interfaces, default port 9000
./safesocket server

# Custom address and port
./safesocket server --host 0.0.0.0 --port 9000

# With encryption
./safesocket server --encrypt RC4 --key mypassword

# With access-key authentication
./safesocket server --require-key --key serverpassword

# Full example
./safesocket server --host 0.0.0.0 --port 9000 \
  --encrypt RC4 --key s3cr3t \
  --require-key --access-key joinpass \
  --max-clients 32 \
  --motd "Welcome! Be respectful." \
  --server-name "MyServer" \
  --log server.log \
  --verbose
```

After startup the server drops into the [Server CLI](#server-cli-commands).

---

### Client Mode

Connects to a running SafeSocket server and drops into the [Client CLI](#client-cli-commands).

```bash
# Minimal — connect to localhost, default port
./safesocket client

# Connect to a remote server
./safesocket client --host 192.168.1.10 --port 9000

# With a nickname
./safesocket client --host 192.168.1.10 --port 9000 --nick Alice

# With encryption (must match server)
./safesocket client --host 192.168.1.10 --port 9000 \
  --encrypt RC4 --key s3cr3t --nick Alice

# With access key (required if server has --require-key)
./safesocket client --host 192.168.1.10 --port 9000 \
  --nick Alice --key joinpass

# Load a saved config file
./safesocket client --config ~/.safesocket.conf
```

---

### Genconfig Mode

Writes the compiled-in defaults to a `.conf` file and prints them to stdout. Useful as a starting point for customisation.

```bash
# Write to default location (safesocket.conf)
./safesocket genconfig

# Write to a specific path
./safesocket genconfig /etc/safesocket/server.conf
```

---

## Command-Line Options

All options override values loaded from the config file.

| Flag | Argument | Applies to | Description |
|------|----------|------------|-------------|
| `--host` | `<ip>` | both | IPv4 address to bind (server) or connect to (client). Default: `127.0.0.1` |
| `--port` | `<port>` | both | TCP port number. Default: `9000` |
| `--nick` | `<name>` | client | Nickname shown to other users. Default: `anonymous` |
| `--encrypt` | `NONE\|XOR\|VIGENERE\|RC4` | both | Payload encryption algorithm. Default: `NONE` |
| `--key` | `<passphrase>` | both | Encryption key (and access key for client) |
| `--config` | `<path>` | both | Path to `.conf` file. Default: `safesocket.conf` |
| `--download-dir` | `<dir>` | client | Directory where received files are saved. Default: `./downloads` |
| `--log` | `<file>` | both | Enable file logging and set log path |
| `--verbose` | — | both | Enable extra diagnostic output |
| `--no-color` | — | both | Disable ANSI colour in terminal output |
| `--max-clients` | `<n>` | server | Maximum simultaneous connections. Default: `64` |
| `--require-key` | — | server | Reject clients that don't supply the correct access key |
| `--access-key` | `<key>` | server | The shared secret clients must send |
| `--motd` | `<text>` | server | Message of the Day sent to each client on connect |
| `--server-name` | `<name>` | server | Display name shown in the startup banner |
| `--help` | — | both | Print usage and exit |

---

## Configuration File

SafeSocket looks for `safesocket.conf` in the working directory unless `--config` points elsewhere. The format is plain `key=value`, one per line. Lines starting with `#` or `;` are comments.

Generate a fully-commented default file:

```bash
./safesocket genconfig safesocket.conf
```

Example file:

```ini
# [network]
host            = 0.0.0.0
port            = 9000
max_clients     = 64
recv_timeout    = 300
connect_timeout = 10

# [identity]
nickname        = anonymous
server_name     = SafeSocket-Server
motd            = Welcome to SafeSocket!

# [security]
encrypt_type    = NONE
encrypt_key     =
require_key     = false
access_key      =

# [transfer]
download_dir    = ./downloads
auto_accept_files = false
max_file_size   = 0

# [logging]
log_to_file     = false
log_file        = safesocket.log
verbose         = false

# [keepalive]
keepalive       = true
ping_interval   = 30

# [misc]
color_output    = true
buffer_size     = 4096
```

### All Configuration Keys

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `host` | string | `127.0.0.1` | Bind address (server) / server IP (client) |
| `port` | int | `9000` | TCP port |
| `max_clients` | int | `64` | Max simultaneous clients (server) |
| `recv_timeout` | int | `300` | Socket receive timeout in seconds; `0` = disabled |
| `connect_timeout` | int | `10` | Seconds to wait for TCP connect (client) |
| `nickname` | string | `anonymous` | Display name sent during handshake (client) |
| `server_name` | string | `SafeSocket-Server` | Server display name in banner (server) |
| `motd` | string | `Welcome to SafeSocket!` | Message of the Day (server) |
| `encrypt_type` | string | `NONE` | Encryption algorithm: `NONE`, `XOR`, `VIGENERE`, `RC4` |
| `encrypt_key` | string | _(empty)_ | Passphrase for encryption |
| `require_key` | bool | `false` | Require clients to authenticate (server) |
| `access_key` | string | _(empty)_ | Shared secret clients must provide (server) |
| `download_dir` | string | `./downloads` | Directory for received files |
| `auto_accept_files` | bool | `false` | Auto-accept incoming file transfers without prompting |
| `max_file_size` | int | `0` | Max accepted file size in bytes; `0` = unlimited |
| `log_to_file` | bool | `false` | Write log messages to `log_file` |
| `log_file` | string | `safesocket.log` | Log file path |
| `verbose` | bool | `false` | Extra diagnostic output to stdout |
| `keepalive` | bool | `true` | Send periodic pings to detect dead connections (server) |
| `ping_interval` | int | `30` | Seconds between keepalive pings (server) |
| `color_output` | bool | `true` | ANSI colour in terminal output |
| `buffer_size` | int | `4096` | Internal I/O buffer size hint in bytes |

### Runtime Changes with /set

Both the server and client support changing config values without restarting:

```
/set <key> <value>
/set port 9001
/set verbose true
/set encrypt_type RC4
/set motd "New message of the day"
```

Use `/config` to see current values and `/confighelp` for a full key listing. Save the current state at any time:

```
/saveconfig safesocket.conf
/loadconfig /path/to/other.conf
```

---

## Encryption

SafeSocket supports four payload encryption modes. Both the server and all clients must use the same algorithm and key.

| Mode | Strength | Notes |
|------|----------|-------|
| `NONE` | None | Plaintext — default |
| `XOR` | Weak | Fast, repeating-key XOR. Obfuscation only |
| `VIGENERE` | Weak | Byte-level Vigenère cipher |
| `RC4` | Moderate | Key-scheduled stream cipher, practical for most uses |

```bash
# Server with RC4
./safesocket server --encrypt RC4 --key mypassword

# Client must match exactly
./safesocket client --host 192.168.1.10 --encrypt RC4 --key mypassword
```

> **Note:** Mismatched encryption or keys will produce garbled or unreadable messages. No external cryptographic libraries are required — all algorithms are implemented in `crypto.cpp`.

---

## Access Control

The server can require a shared secret from every connecting client:

```bash
# Server side
./safesocket server --require-key --access-key supersecret

# Client side — supply the key via --key
./safesocket client --host 192.168.1.10 --key supersecret
```

Clients that send the wrong key or no key are rejected immediately with an `ACCESS DENIED` message. `access_key` and `encrypt_key` are separate fields — you can use both, one, or neither independently.

---

## Server CLI Commands

Once the server is running, the operator controls it via stdin:

| Command | Arguments | Description |
|---------|-----------|-------------|
| `/help` or `/?` | — | Print all available commands |
| `/list` | — | Show a table of all connected clients (ID, nickname, IP, connect time) |
| `/stats` | — | Show server statistics: uptime, bytes sent/received, client count |
| `/broadcast` | `<message>` | Send a message to all connected clients |
| `/msg` | `<id> <message>` | Send a private message to a client by numeric ID |
| `/msgn` | `<nickname> <message>` | Send a private message to a client by nickname |
| `/kick` | `<id> [reason]` | Disconnect a client by ID with an optional reason |
| `/kickn` | `<nickname> [reason]` | Disconnect a client by nickname |
| `/sendfile` | `<id> <filepath>` | Send a file to a specific client |
| `/sendfileall` | `<filepath>` | Send a file to every connected client |
| `/config` | — | Display current configuration values |
| `/confighelp` | — | List all valid configuration keys |
| `/set` | `<key> <value>` | Change a configuration value at runtime |
| `/saveconfig` | `<path>` | Save current configuration to a file |
| `/loadconfig` | `<path>` | Load configuration from a file |
| `/quit` or `/stop` or `/exit` | — | Gracefully shut down the server |

---

## Client CLI Commands

Once connected, the user interacts via stdin:

| Command | Arguments | Description |
|---------|-----------|-------------|
| `/help` or `/?` | — | Print all available commands |
| `/list` | — | Request and display the current client list from the server |
| `/myid` | — | Print your own numeric client ID |
| `/nick` | `<name>` | Change your nickname |
| `/broadcast` | `<message>` | Send a message to all clients |
| `/msg` | `<id> <message>` | Send a private message to a client by numeric ID |
| `/msgn` | `<nickname> <message>` | Send a private message to a client by nickname |
| `/sendfile` | `<id> <filepath>` | Send a local file to another client (routed via server) |
| `/sendfileserver` | `<filepath>` | Upload a local file directly to the server |
| `/config` | — | Display current configuration values |
| `/confighelp` | — | List all valid configuration keys |
| `/set` | `<key> <value>` | Change a configuration value at runtime |
| `/saveconfig` | `<path>` | Save current configuration to a file |
| `/loadconfig` | `<path>` | Load configuration from a file |
| `/quit` or `/exit` or `/disconnect` | — | Disconnect and exit |

> **Tip:** Plain text entered without a leading `/` is sent as a broadcast message to all clients.

---

## File Transfer

### Client sending to another client

```
/sendfile <target_id> /path/to/file.zip
```

The receiving client is prompted to accept or reject. If `auto_accept_files = true` in their config, the file is accepted automatically and saved to their `download_dir`.

### Client uploading to the server

```
/sendfileserver /path/to/report.pdf
```

The file is saved in the server's `download_dir`.

### Server sending to a client

```
/sendfile <client_id> /srv/files/update.zip
```

### Server sending to all clients

```
/sendfileall /srv/files/patch.bin
```

Files are transferred in 64 KB chunks. Progress is printed during the transfer. If the receiver rejects the transfer or the connection drops mid-transfer, the partial file is discarded.

| Setting | Effect |
|---------|--------|
| `auto_accept_files = true` | Never prompt — accept all incoming files |
| `max_file_size = <bytes>` | Reject files larger than this; `0` means unlimited |
| `download_dir = <path>` | Where received files are written; created if absent |

---

## Quick-Start Examples

### LAN chat, no encryption

```bash
# Server (machine A, IP 192.168.1.10)
./safesocket server --host 0.0.0.0 --port 9000

# Client 1 (machine B)
./safesocket client --host 192.168.1.10 --port 9000 --nick Alice

# Client 2 (machine C)
./safesocket client --host 192.168.1.10 --port 9000 --nick Bob
```

### Encrypted private server with access control

```bash
# Server
./safesocket server --host 0.0.0.0 --port 9000 \
  --encrypt RC4 --key topsecret \
  --require-key --access-key joinme \
  --motd "Private channel — authorised users only"

# Client
./safesocket client --host myserver.example.com --port 9000 \
  --nick Alice --encrypt RC4 --key topsecret --key joinme
```

> Use separate `--key` for encrypt key and `--access-key` for join password when both are needed.

### Using a config file

```bash
# Generate defaults
./safesocket genconfig myserver.conf

# Edit myserver.conf, then start
./safesocket server --config myserver.conf
```

### Loopback test on one machine

```bash
# Terminal 1
./safesocket server --host 127.0.0.1 --port 9000

# Terminal 2
./safesocket client --host 127.0.0.1 --port 9000 --nick Alice

# Terminal 3
./safesocket client --host 127.0.0.1 --port 9000 --nick Bob
```
