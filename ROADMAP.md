# SafeSocket — Roadmap

This document is the authoritative record of every past, current, and planned release.
It is updated at the start of each release cycle and again when the release ships.

---

## Release Overview

| Version | Codename | Status | Release Date | Theme |
|---|---|---|---|---|
| [0.0.1](#001--initial) | Initial | Released | 2026-03-15 | Foundation |
| [0.0.2](#002--stability) | Stability | Planned | TBD | Bug fixes & polish |
| [0.0.3](#003--observability) | Observability | Planned | TBD | Logging & diagnostics |
| [0.0.4](#004--expansion) | Expansion | Planned | TBD | Core feature additions |
| [0.0.5](#005--security) | Security | Proposed | TBD | Authenticated encryption |
| [0.0.6](#006--production) | Production | Proposed | TBD | Stability & packaging |

**Status definitions**

| Status | Meaning |
|---|---|
| Released | Shipped, tagged, stable |
| In Progress | Merged to `main`, not yet tagged |
| Planned | Scoped and scheduled, not yet started |
| Proposed | Under consideration, not committed |
| Rejected | Considered and ruled out |

---

---

## 0.0.1 — Initial

**Released:** 2026-03-15
**Status:** Released

> Establishes the complete architectural foundation. Everything in this release is the baseline every future version builds on.

---

### Scope Summary

| Area | Delivered |
|---|---|
| Architecture | Single binary, zero dependencies, C++11, Linux / macOS / Windows |
| Wire Protocol | 20-byte `PacketHeader`, 18 message types, `make_packet()` builders |
| Encryption | XOR, Vigenère, RC4 — unified in-place API, RFC 6229 test vector |
| Server | Multi-client threads, keepalive, auth, broadcast, private msg, file relay |
| Client | Receive thread, peer cache, file send/receive, full CLI |
| Configuration | 24 fields, INI parser, runtime `/set`, sensitive-field masking |
| Tests | ~141 cases across unit / integration / stress, zero-dependency framework |
| Documentation | `usage.md`, `build.md`, `architecture.md`, `CONTRIBUTING.md`, 12 diagrams |

---

### Protocol Constants

| Constant | Value | Purpose |
|---|---|---|
| `MAGIC` | `0x534B5A4F` | Header validation sentinel |
| `HEADER_SIZE` | 20 bytes | Fixed wire header length |
| `FILE_CHUNK` | 65 536 bytes | Maximum file data payload per packet |
| `MAX_CLIENTS` | 64 | Compile-time connection ceiling |
| `SERVER_ID` | `0` | Reserved ID for server-originated packets |
| `BROADCAST_ID` | `0xFFFFFFFF` | Reserved ID meaning "deliver to all" |

---

### Message Types Shipped

| Value | Name | Direction | Payload |
|---|---|---|---|
| 1 | `MSG_CONNECT` | C → S | `[uint16 nick_len][nick][optional key]` |
| 2 | `MSG_DISCONNECT` | C ↔ S | None |
| 3 | `MSG_TEXT` | S → C | UTF-8 text |
| 4 | `MSG_BROADCAST` | C → S → all | UTF-8 text |
| 5 | `MSG_PRIVATE` | C → S → C | UTF-8 text |
| 6 | `MSG_CLIENT_LIST` | S → C | Serialised peer list |
| 7 | `MSG_FILE_START` | C → S → C | `FileStartPayload` + filename |
| 8 | `MSG_FILE_DATA` | C → S → C | Raw chunk bytes (max 64 KB) |
| 9 | `MSG_FILE_END` | C → S → C | None |
| 10 | `MSG_ACK` | S → C | None |
| 11 | `MSG_ERROR` | S → C | UTF-8 error description |
| 12 | `MSG_NICK_SET` | C → S | UTF-8 new nickname |
| 13 | `MSG_SERVER_INFO` | S → C | Server name + MOTD |
| 14 | `MSG_PING` | S → C | None |
| 15 | `MSG_PONG` | C → S | None |
| 16 | `MSG_KICK` | S → C | UTF-8 reason |
| 17 | `MSG_FILE_ACCEPT` | C → S | None |
| 18 | `MSG_FILE_REJECT` | C → S | None |

---

### Encryption Modes

| Mode | Algorithm | Security Level | Key Required |
|---|---|---|---|
| `NONE` | Plaintext | None | No |
| `XOR` | Repeating-key XOR | Obfuscation only | Yes |
| `VIGENERE` | Byte-level Vigenère | Obfuscation only | Yes |
| `RC4` | RC4 stream cipher | Weak (known attacks) | Yes |

---

### Configuration Keys

| Key | Type | Default | Description |
|---|---|---|---|
| `host` | string | `127.0.0.1` | Bind / connect address |
| `port` | int | `9000` | TCP port |
| `max_clients` | int | `64` | Maximum simultaneous connections |
| `recv_timeout` | int | `300` | Socket receive timeout (seconds) |
| `connect_timeout` | int | `10` | Connect attempt timeout (seconds) |
| `nickname` | string | `anonymous` | Client display name |
| `server_name` | string | `SafeSocket` | Server display name |
| `motd` | string | *(empty)* | Message of the day |
| `encrypt_type` | string | `NONE` | Cipher: `NONE`, `XOR`, `VIGENERE`, `RC4` |
| `encrypt_key` | string | *(empty)* | Cipher passphrase — masked in output |
| `require_key` | bool | `false` | Enforce access key on connect |
| `access_key` | string | *(empty)* | Server access key — masked in output |
| `download_dir` | string | `./downloads` | Directory for received files |
| `auto_accept_files` | bool | `false` | Accept incoming files without prompting |
| `max_file_size` | int | `104857600` | Maximum accepted file size (bytes) |
| `log_to_file` | bool | `false` | Write log output to a file |
| `log_file` | string | `safesocket.log` | Log file path |
| `verbose` | bool | `false` | Enable debug-level log output |
| `keepalive` | bool | `true` | Send periodic pings to detect dead clients |
| `ping_interval` | int | `30` | Keepalive ping interval (seconds) |
| `color_output` | bool | `true` | ANSI colour in CLI output |
| `buffer_size` | int | `4096` | Internal I/O buffer size (bytes) |
| `reconnect` | bool | `false` | Reserved — not active in this version |
| `reconnect_max_attempts` | int | `5` | Reserved — not active in this version |

---

### Test Coverage

| Suite | Category | Cases |
|---|---|---|
| `unit/crypto` | XOR, Vigenère, RC4, type conversions | 34 |
| `unit/protocol` | Header layout, constants, builders | 13 |
| `unit/config` | All 24 keys, load/save, edge cases | 38 |
| `unit/network` | `fmt_bytes`, init, listen/connect, raw I/O, packet I/O | 41 |
| `integration/messaging` | Broadcast, private routing, nick change | 14 |
| `integration/auth` | Open server, correct/wrong/absent key | 5 |
| `integration/file_transfer` | Small / empty / multi-chunk, reject, integrity | 8 |
| `stress/clients` | Sequential, concurrent, churn, broadcast storm | 6 |
| `stress/crypto` | 1–4 MB round-trips, 10 000 chunk cycles | 7 |
| **Total** | | **~141** |

---

### Known Limitations

| Limitation | Targeted in |
|---|---|
| IPv4 only — no IPv6 support | Post 0.0.6 |
| No authenticated encryption — RC4/XOR/Vigenère are obfuscation-grade | 0.0.5 |
| No file transfer progress on the CLI | 0.0.4 |
| No automatic reconnection on connection drop | 0.0.4 |
| `g_config` global singleton blocks multi-instance use | 0.0.6 |
| `MAX_CLIENTS = 64` is a compile-time ceiling | 0.0.4 |

---

### Release Metrics

| Lines of production code | Lines of test code | Test cases | External dependencies |
|---|---|---|---|
| ~3 200 | ~2 800 | ~141 | 0 |

---

---

## 0.0.2 — Stability

**Target:** TBD
**Status:** Planned

> Bug-fix release only. No new features. Eliminate race conditions identified after 0.0.1, harden error reporting, and fix test flakiness on slow CI runners.

---

### Goals

| # | Goal | Measurable Outcome |
|---|---|---|
| 1 | Eliminate known race conditions | Zero failures under `TSAN` on Linux and macOS |
| 2 | `Config::set()` never throws | Invalid values log a warning and leave the field unchanged |
| 3 | Actionable error messages | Every `[net]` / `[server]` log line includes the affected address and a suggested action |
| 4 | `genconfig` self-documents | Generated file includes an inline comment per key |

---

### Planned Changes

#### Bug Fixes

| ID | Component | Description | Root Cause |
|---|---|---|---|
| BUG-01 | Server | Deadlock on rapid client disconnect | `sock_close()` called while holding `m_clients_mutex` |
| BUG-02 | Client | Race between `recv_loop` and `disconnect()` | Plain `bool m_running` has no memory-fence guarantee |
| BUG-03 | Config | `stoi` throws `std::invalid_argument` on non-numeric int keys | No `try/catch` around integer parsing |
| BUG-04 | Network | Silent failure on `EINTR`-interrupted reads | `net_recv_raw` returns `false` instead of retrying |

#### Error Message Improvements

| Context | Before | After |
|---|---|---|
| Connection failure | `[net] connect() failed` | `[net] connect() failed: 192.168.1.5:9000 — Connection refused. Is the server running?` |
| Auth failure | *(silent disconnect)* | Client receives `MSG_ERROR`: "Access denied: wrong or missing key" |
| File not found | `[client] cannot open file` | `[client] Cannot open /path/to/file.zip — No such file or directory` |

#### Test Suite

- Replace all `sleep_ms(N)` timing assumptions with polled assertions (maximum 2 s wait)
- Add one regression test per bug fix above
- Add Windows CI leg to the GitHub Actions workflow

---

### Breaking Changes

None. Wire protocol unchanged. Config keys unchanged.

---

### Migration

No migration required from 0.0.1.

---

---

## 0.0.3 — Observability

**Target:** TBD
**Status:** Planned

> Operator-facing improvements. Structured log output, configurable log levels, runtime statistics counters, and an extended `/status` command.

---

### Goals

| # | Goal | Measurable Outcome |
|---|---|---|
| 1 | Machine-readable logs | JSON mode output validates against a schema |
| 2 | Configurable verbosity | Messages below the configured level are suppressed with zero performance cost |
| 3 | Runtime statistics | All counters readable from the operator CLI without restarting the server |

---

### New Config Keys

| Key | Type | Default | Description |
|---|---|---|---|
| `log_level` | string | `INFO` | Minimum level to emit: `ERROR`, `WARN`, `INFO`, `DEBUG` |
| `log_format` | string | `text` | Output format: `text` or `json` |
| `log_max_size_mb` | int | `10` | Rotate log file after this many megabytes |
| `log_max_files` | int | `3` | Number of rotated files to retain |

---

### Log Format Comparison

| Field | Text Mode | JSON Mode |
|---|---|---|
| Timestamp | `[2025-03-14 12:00:01]` | `"ts":"2025-03-14T12:00:01Z"` |
| Level | `[INFO]` | `"level":"INFO"` |
| Module | `[server]` | `"module":"server"` |
| Event | `Client 3 "Alice" connected from 192.168.1.5` | `"event":"connect","id":3,"nick":"Alice","ip":"192.168.1.5"` |

---

### Runtime Statistics Counters

| Counter | Type | Description |
|---|---|---|
| `messages_sent` | `atomic<uint64_t>` | Total outbound packets dispatched |
| `messages_received` | `atomic<uint64_t>` | Total inbound packets processed |
| `bytes_sent` | `atomic<uint64_t>` | Total bytes written to all sockets |
| `bytes_received` | `atomic<uint64_t>` | Total bytes read from all sockets |
| `files_transferred` | `atomic<uint64_t>` | Completed file transfers |
| `auth_failures` | `atomic<uint64_t>` | Rejected `MSG_CONNECT` attempts |
| `uptime_seconds` | `atomic<uint64_t>` | Seconds since `server.start()` was called |

---

### New Server Commands

| Command | Output |
|---|---|
| `/status` | Single-line summary: `[OK] uptime=2h14m · clients=5/64 · msgs=1,204 · tx=3.2 MB · rx=1.8 MB` |
| `/stats` | Full counter table including per-client breakdown |

---

### Breaking Changes

Config file: four new keys added. Existing config files without them continue to work — defaults apply. Wire protocol unchanged.

---

---

## 0.0.4 — Expansion {#004--expansion}

**Target:** TBD
**Status:** Planned

> First minor version increment. Delivers the most-requested capabilities absent from the 0.0.x series: file transfer progress, automatic reconnection, named rooms, and a raised client ceiling.

---

### Goals

| # | Goal | Measurable Outcome |
|---|---|---|
| 1 | File transfer progress | CLI shows percentage, bytes, and transfer speed updated in place |
| 2 | Automatic reconnection | Client retries with exponential back-off; configurable attempt limit |
| 3 | Named rooms | Messages sent in a room are delivered only to members of that room |
| 4 | Raised client ceiling | `MAX_CLIENTS` raised to 256; `Config::max_clients` fully enforced at runtime |

---

### New Message Types

| Value | Name | Direction | Purpose |
|---|---|---|---|
| 19 | `MSG_FILE_PROGRESS` | S → sender | Bytes relayed so far for an active transfer |
| 20 | `MSG_ROOM_JOIN` | C → S | Request to join or create a named room |
| 21 | `MSG_ROOM_LEAVE` | C → S | Leave a room |
| 22 | `MSG_ROOM_MSG` | C → S → room | Text message scoped to a room |
| 23 | `MSG_ROOM_LIST` | S → C | Current room membership snapshot |

---

### New Config Keys

| Key | Type | Default | Description |
|---|---|---|---|
| `reconnect` | bool | `false` | Enable automatic reconnection on unexpected disconnect |
| `reconnect_max_attempts` | int | `5` | Maximum consecutive reconnection attempts |
| `reconnect_delay_ms` | int | `2000` | Initial retry delay (doubles on each failure) |

---

### New CLI Commands

| Side | Command | Description |
|---|---|---|
| Client | `/reconnect` | Force an immediate reconnect attempt |
| Client / Server | `/join <room>` | Join or create a named room |
| Client / Server | `/leave <room>` | Leave a room |
| Client / Server | `/rooms` | List all active rooms and their member counts |

---

### Breaking Changes

| Area | Change | Backwards Compatible |
|---|---|---|
| `MsgType` enum | Values 19–23 added | Yes — old clients ignore unknown types |
| `MSG_CONNECT` payload | 2-byte version prefix added (value `1`) | Yes — server detects old format by `data_len` |
| `MAX_CLIENTS` constant | Raised from 64 to 256 | Yes |

---

### Migration

- **Server operators:** No config changes required. Rooms are opt-in.
- **0.0.x client → 0.0.4 server:** Fully compatible. Server accepts the old `MSG_CONNECT` format.
- **0.0.4 client → 0.0.x server:** `MSG_ROOM_*` replies will not arrive; client handles their absence gracefully.

---

---

## 0.0.5 — Security {#005--security}

**Target:** TBD
**Status:** Proposed

> Replace obfuscation-grade ciphers with an authenticated encryption scheme. No external dependencies — self-contained ChaCha20-Poly1305 and X25519 key exchange.

---

### Goals

| # | Goal | Measurable Outcome |
|---|---|---|
| 1 | Authenticated encryption | All traffic protected by a cipher providing confidentiality, integrity, and authenticity |
| 2 | No pre-shared secret required | Ephemeral Diffie-Hellman handshake on every connection |
| 3 | Legacy mode warnings | Selecting XOR / Vigenère / RC4 emits a visible warning at startup |

---

### Proposed Changes

#### New Cipher Mode

| Property | Detail |
|---|---|
| `EncryptType` value | `CHACHA20` |
| Stream cipher | ChaCha20 — self-contained, ~200 lines |
| Authentication | Poly1305 MAC, 16-byte tag appended per payload |
| Nonce | 12 bytes derived from `packet_counter XOR sender_id` |
| Granularity | `MSG_FILE_DATA` chunks authenticated individually |

#### Key Exchange

| Property | Detail |
|---|---|
| Algorithm | X25519 (Curve25519) |
| Implementation | Self-contained, ~300 lines, zero external code |
| New message types | `MSG_DH_HELLO`, `MSG_DH_RESPONSE` |
| Session key | Derived from shared secret; pre-shared `encrypt_key` usable as additional KDF input |
| Logging | Session keys never written to log at any level |

#### Legacy Cipher Policy

| Cipher | New Behaviour |
|---|---|
| `XOR` | Warning banner at startup; requires `--insecure` flag to suppress |
| `VIGENERE` | Warning banner at startup; requires `--insecure` flag to suppress |
| `RC4` | Warning banner at startup; requires `--insecure` flag to suppress |
| `NONE` | Warning banner at startup when `require_key = true`; no warning otherwise |

---

### Open Questions

| Question | Decision Needed By |
|---|---|
| Should `CHACHA20` become the new default `encrypt_type`? | Start of 0.0.5 cycle |
| Should X25519 live in `crypto.cpp` or a dedicated `dh.cpp`? | Start of 0.0.5 cycle |
| Counter-based nonces are safe per-connection — document the session boundary contract | Before implementation |

---

---

## 0.0.6 — Production {#006--production}

**Target:** TBD
**Status:** Proposed

> API and protocol stability declaration. Formal protocol versioning, comprehensive hardening, and distributable packages for all supported platforms.

---

### Goals

| # | Goal | Measurable Outcome |
|---|---|---|
| 1 | No known vulnerabilities in default configuration | Clean audit against OWASP top-10 network application risks |
| 2 | Stable protocol | Wire protocol v2 documented, versioned, and frozen |
| 3 | Test coverage | 95%+ line coverage on production code (`gcov` / `llvm-cov`) |
| 4 | Distributable packages | Pre-built binaries for all four target platforms |

---

### Protocol v2 Changes

| Field | v1 (current) | v2 |
|---|---|---|
| `MAGIC` | `0x534B5A4F` | `0x534B5A02` |
| Byte order | Host byte order (undocumented) | Little-endian, explicitly specified |
| Version negotiation | None | `MSG_CONNECT` payload carries version field; server and client agree on highest mutual version |

---

### Hardening Items

| Item | Description |
|---|---|
| Server-side file size enforcement | Server aborts relay if cumulative chunk bytes exceed `max_file_size` |
| Per-client rate limiting | Configurable `max_msgs_per_second`; excess dropped with `MSG_ERROR` reply |
| Nickname validation | Printable ASCII only, max 32 characters, enforced server-side |
| Log injection prevention | All string fields sanitised before being written to log output |
| `g_config` singleton removal | Refactor to allow multiple Server/Client instances per process |

---

### Packaging Targets

| Platform | Format | Build System |
|---|---|---|
| Linux x86-64 | `.deb`, `.tar.gz` | `dpkg-buildpackage`, GitHub Actions |
| Linux arm64 | `.deb`, `.tar.gz` | GitHub Actions (QEMU cross) |
| macOS arm64 | `.tar.gz` | GitHub Actions (macOS runner) |
| Windows x86-64 | `.zip` (MSVC binary) | GitHub Actions (Windows runner) |

---

---

## Rejected Proposals

| Proposal | Raised | Reason |
|---|---|---|
| Use OpenSSL for encryption | 0.0.1 planning | Violates the zero-dependency constraint. Self-contained crypto is a core project value. |
| UDP transport | 0.0.1 planning | TCP reliability is a design requirement. A UDP path would require reimplementing the entire reliability layer. |
| Shared library (`libsafesocket.so`) | 0.0.1 planning | Violates the single-binary constraint. Embedding use cases can link the `.cpp` files directly. |
| Python bindings | 0.0.1 planning | The maintenance surface of external language bindings is disproportionate to the benefit at this stage. |

---

## Long-Range Ideas

Items with no assigned version or owner. Recorded to prevent loss across planning cycles.

| Idea | Notes |
|---|---|
| IPv6 support | Dual-stack `AF_INET6` behind the `net_*` abstraction layer |
| Message history replay | Server buffers the last N broadcasts; new clients receive them on join |
| File transfer resume | Track byte offset across disconnects; restart from the last confirmed chunk |
| Syslog integration | `LOG_DAEMON` facility on Linux; Windows Event Log on Windows |
| Binary delta transfer | Send only changed bytes using a rolling-checksum diff (rsync-style) |
| Embedded web UI | Optional hand-rolled HTTP/1.1 server serving a minimal browser chat interface — zero new dependencies |
