# Architecture

This document describes the internal design of SafeSocket: how the modules fit together, the threading model, the wire protocol, the encryption pipeline, and the data flow for every major operation.

## Table of Contents

- [High-Level Overview](#high-level-overview)
- [Module Map](#module-map)
- [Threading Model](#threading-model)
  - [Server Threads](#server-threads)
  - [Client Threads](#client-threads)
- [Wire Protocol](#wire-protocol)
  - [PacketHeader](#packetheader)
  - [Payload Layouts](#payload-layouts)
  - [Message Types](#message-types)
- [Encryption Pipeline](#encryption-pipeline)
- [Connection Lifecycle](#connection-lifecycle)
  - [Server Side](#server-side)
  - [Client Side](#client-side)
- [Messaging Flow](#messaging-flow)
- [File Transfer Flow](#file-transfer-flow)
- [Keepalive and Disconnect Detection](#keepalive-and-disconnect-detection)
- [Configuration System](#configuration-system)
- [Network Abstraction Layer](#network-abstraction-layer)
- [Key Data Structures](#key-data-structures)
- [Diagrams](#diagrams)

---

## High-Level Overview

SafeSocket is a single-binary, multi-client TCP chat and file-transfer application written in C++11. The same binary is used for both the server and client roles — the first command-line argument selects the mode.

```
safesocket server [options]   →  runs Server
safesocket client [options]   →  runs Client
safesocket genconfig          →  writes a default .conf file
```

All platform differences are isolated in `network.cpp` and `network.hpp`. The rest of the codebase is portable C++11.

---

## Module Map

```
main.cpp
  ├── config.cpp / config.hpp      Global runtime configuration (g_config singleton)
  ├── network.cpp / network.hpp    Cross-platform socket abstraction
  │     └── crypto.cpp / crypto.hpp    In-place payload encryption
  ├── protocol.hpp                 Wire protocol constants, PacketHeader, MsgType
  ├── server.cpp / server.hpp      Multi-threaded server
  └── client.cpp / client.hpp      TCP client with background receive thread
```

**Dependency rules:**
- `protocol.hpp` has no dependencies beyond the C++ standard library.
- `crypto.hpp` has no dependencies beyond the C++ standard library.
- `config.hpp` depends only on `crypto.hpp`.
- `network.hpp` depends on `protocol.hpp`, `crypto.hpp`, and `config.hpp`.
- `server.hpp` and `client.hpp` depend on `network.hpp`, `protocol.hpp`, and `config.hpp`.
- `main.cpp` depends on everything.

There are no circular dependencies and no external libraries.

---

## Threading Model

### Server Threads

The server spawns four categories of threads:

```
Main thread
  ├── Accept thread         (1 thread, permanent)
  ├── Client handler thread (1 per connected client, detached)
  └── Keepalive thread      (1 thread, permanent, optional)
```

| Thread | Role | Lifetime |
|--------|------|---------|
| **Main** | Runs `Server::run_cli()`, reads operator commands from stdin | Until `/quit` |
| **Accept** | Calls `accept()` in a loop, allocates `ClientInfo`, spawns handler threads | Until `m_running = false` |
| **Client handler** | Runs `Server::client_loop()` for one client: receives packets, dispatches to `handle_packet()` | Until client disconnects or `ClientInfo::alive = false` |
| **Keepalive** | Sleeps for `ping_interval` seconds, then sends `MSG_PING` to every live client | Until `m_running = false` |

All access to the shared client map (`m_clients`) is protected by `m_clients_mutex`. Public methods never acquire this lock internally in a way that can deadlock — caller-held locks are not expected.

Client handler threads are **detached** immediately after creation. Cleanup on disconnect is handled inside `client_loop` itself (closing the socket, removing the entry from the map, broadcasting the departure notice).

### Client Threads

```
Main thread
  └── Recv thread   (1 thread, permanent while connected)
```

| Thread | Role | Lifetime |
|--------|------|---------|
| **Main** | Runs `Client::run_cli()`, reads user commands from stdin, sends packets | Until `/quit` or disconnection |
| **Recv** | Runs `Client::recv_loop()`, receives all incoming packets, calls `handle_packet()` | Until `m_running = false` |

Shared state between the two threads:
- `m_peers` (peer cache) — guarded by `m_peers_mutex`.
- `m_connected` and `m_running` — `std::atomic<bool>`, safe to read without a lock.
- `m_my_id` — written once by the recv thread (on first `MSG_CONNECT` response), then read-only.

---

## Wire Protocol

Every transmission consists of a fixed 20-byte `PacketHeader` followed immediately by `data_len` payload bytes.

### PacketHeader

```
Offset  Size  Field       Description
     0     4  magic       Always 0x534B5A4F. Validated on every recv.
     4     4  msg_type    One of the MsgType enum values (uint32_t).
     8     4  sender_id   Numeric ID of the originator, or 0 for SERVER_ID.
    12     4  target_id   Numeric ID of the recipient, or 0xFFFFFFFF for BROADCAST_ID.
    16     4  data_len    Byte length of the payload that follows this header.
```

The struct is declared with `#pragma pack(push, 1)` so there is no compiler-inserted padding. Total size is always exactly 20 bytes.

**Special ID values:**

| Constant | Value | Meaning |
|----------|-------|---------|
| `SERVER_ID` | `0` | Packet originated from or is directed to the server process |
| `BROADCAST_ID` | `0xFFFFFFFF` | Deliver to every connected client |

### Payload Layouts

**`MSG_CONNECT`**
```
[uint16_t nick_len][nick bytes (nick_len)][optional access_key bytes]
```
No null terminators. The access key occupies all remaining bytes after the nickname.

**`MSG_FILE_START`**
```
[uint64_t file_size][uint32_t filename_len][filename bytes (filename_len)]
```
No null terminator on the filename.

**`MSG_CLIENT_LIST`**
```
[uint32_t count]
  for each entry:
    [uint32_t id][uint16_t nick_len][nick bytes (nick_len)]
```

**`MSG_TEXT` / `MSG_BROADCAST` / `MSG_PRIVATE` / `MSG_NICK_SET` / `MSG_ACK` / `MSG_ERROR` / `MSG_KICK` / `MSG_SERVER_INFO`**

Plain UTF-8 string, not null-terminated. Length is given by `data_len`.

**`MSG_FILE_DATA`**
Raw binary chunk, up to 64 KiB (`FILE_CHUNK = 65536`).

**`MSG_DISCONNECT` / `MSG_FILE_END` / `MSG_FILE_ACCEPT` / `MSG_FILE_REJECT` / `MSG_PING` / `MSG_PONG`**
No payload (`data_len = 0`).

### Message Types

| Value | Name | Direction | Description |
|-------|------|-----------|-------------|
| 1 | `MSG_CONNECT` | C→S | Client hello with nickname and optional access key |
| 2 | `MSG_DISCONNECT` | either | Graceful disconnect notification |
| 3 | `MSG_TEXT` | either | General text message |
| 4 | `MSG_BROADCAST` | either | Text to all clients |
| 5 | `MSG_PRIVATE` | either | Text to one client |
| 6 | `MSG_CLIENT_LIST` | S→C or C→S (request) | Serialised list of online clients |
| 7 | `MSG_FILE_START` | either | Begin file transfer; contains name and size |
| 8 | `MSG_FILE_DATA` | either | Raw file chunk |
| 9 | `MSG_FILE_END` | either | End of file transfer |
| 10 | `MSG_ACK` | either | Generic acknowledgment |
| 11 | `MSG_ERROR` | S→C | Human-readable error string |
| 12 | `MSG_NICK_SET` | C→S | Request nickname change |
| 13 | `MSG_SERVER_INFO` | S→C | Server name and MOTD |
| 14 | `MSG_PING` | S→C | Keepalive ping |
| 15 | `MSG_PONG` | C→S | Keepalive pong reply |
| 16 | `MSG_KICK` | S→C | Server kicks client with optional reason |
| 17 | `MSG_FILE_ACCEPT` | C→S | Receiver accepts an incoming file |
| 18 | `MSG_FILE_REJECT` | C→S | Receiver rejects an incoming file |

---

## Encryption Pipeline

Encryption is applied **in-place on the payload buffer** before it is passed to `net_send_raw()`, and decryption is applied in-place immediately after `net_recv_raw()` returns the payload. The `PacketHeader` itself is never encrypted.

**Send path:**
```
application data
  → crypto_encrypt(buf, type, key)    [in-place, noop for NONE]
  → net_send_raw(header)
  → net_send_raw(payload)
  → TCP
```

**Receive path:**
```
TCP
  → net_recv_raw(header, 20 bytes)
  → validate magic == 0x534B5A4F       [drop connection on failure]
  → net_recv_raw(payload, data_len bytes)
  → crypto_decrypt(buf, type, key)    [in-place, noop for NONE]
  → handle_packet(hdr, payload)
```

| Algorithm | Encryption | Decryption | Notes |
|-----------|-----------|-----------|-------|
| `NONE` | no-op | no-op | Plaintext |
| `XOR` | `buf[i] ^= key[i % len]` | identical (self-inverse) | Fast, weak |
| `VIGENERE` | `buf[i] = (buf[i] + key[i % len]) % 256` | `buf[i] = (buf[i] - key[i % len] + 256) % 256` | Separate encrypt/decrypt paths |
| `RC4` | KSA + PRGA XOR | identical (self-inverse) | Stronger; key-scheduled |

All algorithms are implemented in `crypto.cpp` with no external dependencies. The key and algorithm are read from `g_config` at the call site in `net_send_packet` and `net_recv_packet`.

## Connection Lifecycle

### Server Side

```
net_listen(host, port)
  └── SO_REUSEADDR + TCP_NODELAY set on listen socket

Accept loop:
  accept() → new socket
    TCP_NODELAY set on client socket
    allocate ClientInfo { id, sock, ip, nickname, alive=true }
    spawn client_loop(ClientInfo*)  →  detach thread

client_loop:
  send MSG_SERVER_INFO (MOTD)
  loop:
    net_recv_packet(sock, hdr, payload, enc, key)
    if error / recv==0  →  break
    handle_packet(ci, hdr, payload)
  close(sock)
  remove from m_clients
  broadcast "[nick] left the server."
```

### Client Side

```
net_connect(host, port, timeout)
  └── non-blocking connect() + select() timeout
  └── restore blocking mode after connect

spawn recv_loop thread

send MSG_CONNECT [nick_len][nick][access_key]

recv_loop:
  loop:
    net_recv_packet(sock, hdr, payload, enc, key)
    if error  →  break
    handle_packet(hdr, payload)
  m_connected = false

CLI loop (main thread):
  read stdin
  parse command / send packet
  on /quit  →  send MSG_DISCONNECT  →  close socket  →  join recv_loop
```

## Messaging Flow

### Broadcast

```
Client A  →  MSG_BROADCAST (target=BROADCAST_ID)  →  Server
Server    →  MSG_BROADCAST (sender=A_ID)           →  Client B
Server    →  MSG_BROADCAST (sender=A_ID)           →  Client C
```

### Private message

```
Client A  →  MSG_PRIVATE (target=B_ID)  →  Server
Server    →  look up B_ID in m_clients
Server    →  MSG_PRIVATE (sender=A_ID)  →  Client B only
```

### Server-originated broadcast

```
Server CLI: /broadcast text
Server  →  MSG_BROADCAST (sender=SERVER_ID)  →  all clients
```

### Nickname change

```
Client B  →  MSG_NICK_SET payload="Robert"  →  Server
Server    →  update ClientInfo.nickname
Server    →  MSG_BROADCAST "Bob is now known as Robert"  →  all clients
```

---

## File Transfer Flow

```
Sender  →  MSG_FILE_START [file_size][filename_len][filename]  →  Server
Server  →  MSG_FILE_START (forwarded)                          →  Receiver

Receiver  →  MSG_FILE_ACCEPT  →  Server  →  Sender     (or MSG_FILE_REJECT)

loop (64 KB chunks):
  Sender  →  MSG_FILE_DATA [chunk]  →  Server  →  Receiver

Sender    →  MSG_FILE_END  →  Server  →  Receiver
Receiver  →  MSG_ACK       →  Server  →  Sender
```

Key behaviours:
- The server **routes** file packets between clients — it does not buffer the entire file in memory; chunks flow through as they arrive.
- If `auto_accept_files = true` on the receiver, `MSG_FILE_ACCEPT` is sent automatically without user interaction.
- If `max_file_size > 0` and `file_size` exceeds it, the receiver sends `MSG_FILE_REJECT` immediately.
- On any send or receive error mid-transfer, the partial destination file is closed and removed.
- Chunk size is fixed at 64 KiB (`FILE_CHUNK = 65536` in `protocol.hpp`).

---

## Keepalive and Disconnect Detection

The server's keepalive thread wakes every `ping_interval` seconds and sends `MSG_PING` to each client in the map. Clients respond with `MSG_PONG` from their recv loop.

Dead connections are detected passively: `net_recv_packet()` returns `false` when `recv()` returns 0 (clean close) or a negative error. The client handler thread breaks its loop, closes the socket, and removes the entry.

```
Keepalive thread  →  MSG_PING  →  Client (alive)
                               ←  MSG_PONG

net_recv_packet() == false  (dead client)
  →  ClientInfo.alive = false
  →  sock_close(sock)
  →  remove from m_clients
  →  broadcast "[nick] disconnected"
```

---

## Configuration System

`g_config` is a global `Config` struct instance defined in `config.cpp` and `extern`-declared in `config.hpp`. Every translation unit that includes `config.hpp` shares the same instance.

**Loading priority (later overrides earlier):**

1. Compiled-in member-initialiser defaults in `Config`.
2. `Config::load(path)` — parses a plain `key=value` file. Unknown keys emit a warning to stderr and are skipped. Booleans accept `true/1/yes/on`; integers use `std::stoi` with fallback to current value.
3. Command-line `--flag` overrides applied in `main.cpp` after `load()`.

**Runtime mutation via `/set`:**

Both CLIs call `Config::set(key, value)` which accepts the same key names as the config file. Changes take effect immediately — no restart required — because all code reads from `g_config` at call time rather than caching values at startup.

**Sensitive fields** (`encrypt_key`, `access_key`) are masked as `***hidden***` in `Config::print()` and `Config::get()` output when non-empty.

---

## Network Abstraction Layer

`network.hpp` / `network.cpp` provide a single API that compiles on both Windows (Winsock2) and POSIX (BSD sockets) without `#ifdef` pollution leaking into the rest of the codebase.

| Concern | Windows | POSIX |
|---------|---------|-------|
| Socket type (`sock_t`) | `SOCKET` (unsigned) | `int` |
| Invalid socket (`INVALID_SOCK`) | `INVALID_SOCKET` | `-1` |
| Close socket (`sock_close`) | `closesocket()` | `close()` |
| Error code (`sock_errno()`) | `WSAGetLastError()` | `errno` |
| Non-blocking mode | `ioctlsocket(FIONBIO)` | `fcntl(O_NONBLOCK)` |
| Prevent SIGPIPE | n/a | `signal(SIGPIPE, SIG_IGN)` in `net_init()` |
| Send flag | none | `MSG_NOSIGNAL` |
| Initialisation | `WSAStartup(2.2)` | no-op |
| Cleanup | `WSACleanup()` | no-op |

**Function summary:**

| Function | Description |
|----------|-------------|
| `net_init()` | Must be called once before any other `net_*` call |
| `net_cleanup()` | Must be called once at exit |
| `net_listen(host, port, backlog)` | Create, bind, and listen; sets `SO_REUSEADDR` + `TCP_NODELAY` |
| `net_accept(server_sock, peer_ip)` | Block until a client connects; sets `TCP_NODELAY` on the new socket |
| `net_connect(host, port, timeout)` | Non-blocking connect with `select()` timeout; restores blocking mode |
| `net_set_nonblocking(s, bool)` | Toggle blocking/non-blocking |
| `net_set_nodelay(s)` | Disable Nagle's algorithm (`TCP_NODELAY`) |
| `net_set_reuseaddr(s)` | Set `SO_REUSEADDR` |
| `net_send_raw(s, data, len)` | Loop until all `len` bytes are sent |
| `net_recv_raw(s, buf, len)` | Loop until exactly `len` bytes are received |
| `net_send_packet(...)` | Encrypt payload, build header, call `net_send_raw` twice |
| `net_recv_packet(...)` | Call `net_recv_raw` twice, validate magic, decrypt payload |
| `net_error_str()` | Human-readable last socket error |
| `fmt_bytes(n)` | Format a byte count: B / KB / MB / GB / TB |

---

## Key Data Structures

### `PacketHeader` (protocol.hpp)
20-byte packed struct. The foundation of every transmission.

### `FileStartPayload` (protocol.hpp)
12-byte packed struct prefixing the filename in `MSG_FILE_START` packets.

### `Config` (config.hpp)
Aggregates all 24 configurable parameters. The global `g_config` instance is shared across all modules. Thread-safe for concurrent reads; writes are restricted to the CLI thread.

### `ClientInfo` (server.hpp)
Heap-allocated per connected client. Holds the socket, IP, nickname, authentication state, and the client's handler thread. Owned by the `Server::m_clients` map.

### `PeerInfo` (client.hpp)
Lightweight snapshot of another client's ID and nickname. Populated from `MSG_CLIENT_LIST` responses and used by the client CLI to resolve nicknames to IDs for `/msgn` and `/sendfile`.

### `Server` (server.hpp)
Owns the listen socket, the `m_clients` map, the accept and keepalive threads, and the log file. All public messaging methods are safe to call from the CLI thread while handler threads are running.

### `Client` (client.hpp)
Owns the connected socket, the recv thread, and the `m_peers` cache. The `m_connected` and `m_running` atomics allow the CLI thread and recv thread to coordinate shutdown without a mutex.
