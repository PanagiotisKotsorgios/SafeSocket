# Contributing to SafeSocket

Thank you for taking the time to contribute. This document is the authoritative guide for anyone who wants to submit code, tests, documentation, or bug reports. Read it completely before opening a pull request - reviewers will refer to specific sections when requesting changes.

## Table of Contents

1. [Project Philosophy](#1-project-philosophy)
2. [Repository Layout](#2-repository-layout)
3. [Setting Up a Development Environment](#3-setting-up-a-development-environment)
4. [Branching and Commit Strategy](#4-branching-and-commit-strategy)
5. [C++ Coding Standards](#5-c-coding-standards)
   - 5.1 [Language Version and Portability](#51-language-version-and-portability)
   - 5.2 [Naming Conventions](#52-naming-conventions)
   - 5.3 [File and Header Structure](#53-file-and-header-structure)
   - 5.4 [Types and Includes](#54-types-and-includes)
   - 5.5 [Functions and Methods](#55-functions-and-methods)
   - 5.6 [Error Handling](#56-error-handling)
   - 5.7 [Threading Rules](#57-threading-rules)
   - 5.8 [Memory Management](#58-memory-management)
   - 5.9 [Platform Abstraction](#59-platform-abstraction)
   - 5.10 [Formatting and Layout](#510-formatting-and-layout)
6. [Documentation Standards](#6-documentation-standards)
   - 6.1 [File Headers](#61-file-headers)
   - 6.2 [Class Documentation](#62-class-documentation)
   - 6.3 [Struct and Field Documentation](#63-struct-and-field-documentation)
   - 6.4 [Function Documentation](#64-function-documentation)
   - 6.5 [Inline Comments](#65-inline-comments)
   - 6.6 [Section Dividers](#66-section-dividers)
   - 6.7 [What Not to Comment](#67-what-not-to-comment)
7. [Protocol Changes](#7-protocol-changes)
8. [Crypto Module Rules](#8-crypto-module-rules)
9. [Network Layer Rules](#9-network-layer-rules)
10. [Config System Rules](#10-config-system-rules)
11. [Testing Requirements](#11-testing-requirements)
12. [Pull Request Checklist](#12-pull-request-checklist)
13. [Reporting Bugs](#13-reporting-bugs)

---

## 1. Project Philosophy

SafeSocket has four non-negotiable design constraints that every contribution must respect:

**Zero external dependencies.** The entire codebase - crypto, networking, configuration parsing, CLI — is implemented from scratch. Do not add `#include` directives for third-party libraries. Do not add entries to a package manager. If you need a data structure that the standard library does not provide in C++11, implement it yourself within the project.

**C++11 everywhere.** The target compiler baseline is GCC 4.8 / Clang 3.4 / MSVC 2015. No C++14 features (`std::make_unique`, generic lambdas, variable templates), no C++17 features (`if constexpr`, structured bindings, `std::optional`), no compiler extensions.

**Single binary.** `safesocket server|client|genconfig [options]` is the entire user interface. No installer, no shared library, no daemon helper. Contributions that split the binary or add build-time feature flags will be rejected unless there is a compelling documented reason.

**POSIX and Win32 parity.** Every feature must work identically on Linux, macOS, and Windows. Platform-specific code belongs exclusively in `network.cpp` behind the `net_*` abstraction layer. Any PR that introduces `#ifdef _WIN32` outside that file requires an exceptional justification.

---

## 2. Repository Layout

```
safesocket/
├── main.cpp          Entry point — argument parsing, mode dispatch
├── server.cpp/.hpp   Server class, accept loop, client-handler threads
├── client.cpp/.hpp   Client class, recv loop, CLI
├── network.cpp/.hpp  Platform socket abstraction (POSIX / Winsock2)
├── protocol.hpp      Wire format: PacketHeader, MsgType enum, make_packet()
├── crypto.cpp/.hpp   XOR, Vigenère, RC4 — encrypt/decrypt API
├── config.cpp/.hpp   Config struct, set/get, load/save
└── tests/            Full test suite (see tests/README.md)
```

The rule of thumb is: if a change touches more than two `.cpp` files it probably belongs in a new file of its own. Each source file should have a single clear responsibility that you can state in one sentence.

---

## 3. Setting Up a Development Environment

### Linux / macOS

```bash
git clone https://github.com/PanagiotisKotsorgios/SafeSocket.git
cd safesocket

# Build
g++ -std=c++11 -Wall -Wextra -pthread -o safesocket \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp

# Build with debug info and sanitisers (strongly recommended for development)
g++ -std=c++11 -Wall -Wextra -pthread -g \
    -fsanitize=address,undefined \
    -o safesocket_dbg \
    main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp
```

### Windows (MSVC)

```bat
cl /std:c++17 /EHsc /W3 /Zi ^
   main.cpp server.cpp client.cpp network.cpp crypto.cpp config.cpp ^
   /Fe:safesocket.exe /link ws2_32.lib
```

### Build the Tests

```bash
cd tests && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSS_ASAN=ON
cmake --build . --parallel
ctest --output-on-failure
```

Run sanitisers (`-DSS_ASAN=ON`) for every PR that touches threading or socket code. Reviewers will ask for a clean ASAN/TSAN run log if your PR modifies `server.cpp`, `client.cpp`, or `network.cpp`.

---

## 4. Branching and Commit Strategy

### Branch naming

| Type | Pattern | Example |
|---|---|---|
| Feature | `feature/<short-slug>` | `feature/file-progress-bar` |
| Bug fix | `fix/<short-slug>` | `fix/rc4-empty-key-crash` |
| Docs | `docs/<short-slug>` | `docs/config-key-table` |
| Refactor | `refactor/<short-slug>` | `refactor/server-accept-loop` |
| Test | `test/<short-slug>` | `test/vigenere-wrap-around` |

Never commit directly to `main`. Open a pull request and get at least one approval before merging.

### Commit messages

Follow the conventional commit format:

```
<type>(<scope>): <imperative-mood summary under 72 chars>

<optional body — wrap at 80 chars, explain WHY not what>

<optional footer — refs issues, breaking changes>
```

**Types:** `feat`, `fix`, `refactor`, `test`, `docs`, `style`, `perf`, `build`

**Scopes:** `crypto`, `network`, `protocol`, `server`, `client`, `config`, `tests`, `docs`

```
# Good
feat(crypto): add RC4 test vector for key "Key" (RFC 6229)
fix(server): release client mutex before calling sock_close()
refactor(network): extract getsockname port-probe into free_port()

# Bad — vague, past tense, no scope
Fixed bug
Updated stuff
Changes to server code
```

Each commit should compile and pass tests on its own. Squash fixup commits (`fix typo`, `oops`, `wip`) before opening a PR.

---

## 5. C++ Coding Standards

### 5.1 Language Version and Portability

Use C++11 features freely and prefer them over C-style equivalents:

```cpp
// Prefer
std::vector<uint8_t> buf(HEADER_SIZE + data_len);
auto it = m_clients.find(id);
std::lock_guard<std::mutex> lock(m_clients_mutex);

// Avoid
uint8_t* buf = (uint8_t*)malloc(HEADER_SIZE + data_len);
std::map<uint32_t, ClientInfo*>::iterator it = m_clients.find(id);
```

Do not use:
- `auto` for non-obvious return types (e.g., `auto x = some_func();` where the type is unclear from context)
- Range-based `for` on raw C arrays unless the size is statically known
- `nullptr` in place of `0` for integer comparisons (fine for pointers; confusing for socket handles)
- `std::thread` with a raw function pointer where a lambda with capture would be clearer

### 5.2 Naming Conventions

| Entity | Convention | Example |
|---|---|---|
| Class / struct | `PascalCase` | `ClientInfo`, `Server`, `RC4State` |
| Enum type | `PascalCase` | `EncryptType`, `MsgType` |
| Enum value | `UPPER_SNAKE` | `MSG_CONNECT`, `EncryptType::RC4` |
| Free function | `lower_snake` | `net_send_packet`, `fmt_bytes` |
| Method (public) | `lower_snake` | `client.connect()`, `server.start()` |
| Method (private) | `lower_snake` | `recv_loop()`, `accept_loop()` |
| Member variable | `m_lower_snake` | `m_clients`, `m_running`, `m_sock` |
| Local variable | `lower_snake` | `peer_ip`, `data_len`, `chunk_size` |
| Constant / `#define` | `UPPER_SNAKE` | `MAGIC`, `FILE_CHUNK`, `SERVER_ID` |
| Template parameter | `T` or `PascalCase` | `T`, `InputIt` |

Prefix private member variables with `m_` without exception. This makes the boundary between local variables and member state immediately visible in method bodies, which matters a great deal in the heavily-threaded Server and Client classes.

### 5.3 File and Header Structure

Every `.hpp` file must begin with `#pragma once`. Do not use include guards.

Order of includes:
1. The paired header (e.g., `server.cpp` includes `server.hpp` first)
2. Other project headers
3. Standard library headers (alphabetical within this group)
4. Platform-specific headers (inside `#ifdef _WIN32` / `#else` blocks)

```cpp
// server.cpp
#include "server.hpp"        // 1. paired header
#include "crypto.hpp"        // 2. other project headers
#include "network.hpp"
#include "protocol.hpp"
#include <algorithm>         // 3. standard library (alphabetical)
#include <cstring>
#include <sstream>
#include <string>
```

Do not include headers speculatively. Every `#include` must be justified by a direct use in that translation unit.

### 5.4 Types and Includes

Use fixed-width integer types for all protocol-facing data:

```cpp
// Wire protocol, file sizes, IDs — use fixed-width types
uint32_t sender_id;
uint64_t file_size;
uint8_t  byte_val;

// Local loop counters — plain int is fine
for (int i = 0; i < 256; ++i) { ... }

// Sizes and counts — use size_t
size_t klen = key.size();
```

Never use `long` or `unsigned long` for protocol fields. Their width varies across platforms (32-bit on Windows, 64-bit on most 64-bit Linux). Use `int32_t` / `uint32_t` / `int64_t` / `uint64_t` explicitly.

Cast through `static_cast` at type boundaries. Do not use C-style casts except for the platform socket `(struct sockaddr*)` patterns that are imposed by the POSIX API:

```cpp
// Good
buf[i] = static_cast<uint8_t>((buf[i] + shift) & 0xFF);
int shift = static_cast<int>(static_cast<uint8_t>(key[i % klen]));

// Acceptable (POSIX API requirement)
getsockname(s, (struct sockaddr*)&addr, &len);

// Bad
buf[i] = (uint8_t)((buf[i] + shift) & 0xFF);
```

### 5.5 Functions and Methods

**Single responsibility.** Each function does one thing. If you find yourself writing a function that has a comment block explaining what part A does followed by what part B does, split it into two functions.

**Keep functions short.** Most functions in the codebase are under 40 lines. Functions longer than 80 lines are a code smell. If you write one, justify it in the PR.

**No output parameters when a return value is natural.** Exceptions: `net_accept(sock, peer_ip)` uses an output parameter for the IP string because the function must return a `sock_t`.

```cpp
// Good — result is the return value
std::string encrypt_type_name(EncryptType t);
bool net_send_raw(sock_t s, const void* data, size_t len);

// Acceptable — sock_t is the primary result; ip is secondary
sock_t net_accept(sock_t server_sock, std::string& peer_ip);
```

**Default arguments** are permitted on free functions and constructors where they reduce call-site noise without obscuring intent. Do not use default arguments on virtual functions.

**`const` correctness.** Mark every method that does not mutate visible state as `const`. Mark every function parameter that is not mutated as `const`. Mark every local variable that is not reassigned as `const`.

```cpp
// Good
std::string Config::get(const std::string& key) const;
bool net_set_nodelay(sock_t s);                // s itself is a handle; not const by convention
void crypto_encrypt(std::vector<uint8_t>& buf, // buf IS mutated; not const
                    EncryptType type,
                    const std::string& key);   // key is not mutated; const
```

### 5.6 Error Handling

SafeSocket does not use exceptions for runtime errors. Error conditions are reported through return values:

- Functions that can fail return `bool` (`true` = success, `false` = failure).
- Functions that return a resource return the resource or a sentinel on failure (`INVALID_SOCK`, `nullptr`, empty string).
- Fatal programmer errors (wrong arguments, invariant violations in debug builds) may use `assert()`.

Do **not** throw exceptions in the network layer, the crypto layer, or any code path that runs on a non-main thread. An uncaught exception on a `std::thread` that was `detach()`ed terminates the process with no diagnostic output.

```cpp
// Good
bool net_connect(const std::string& host, int port, int timeout_sec) {
    // ...
    if (result < 0) {
        std::cerr << "[net] connect() failed: " << net_error_str() << "\n";
        sock_close(s);
        return INVALID_SOCK;
    }
    return s;
}

// Bad — exception propagates out of a detached thread
void Server::client_loop(ClientInfo* ci) {
    if (!some_condition)
        throw std::runtime_error("oops");   // NEVER do this
}
```

Log errors to `std::cerr` with a bracketed module prefix (`[net]`, `[server]`, `[crypto]`, `[config]`) and a newline. Do not use `std::endl` (it flushes; use `"\n"`). Suppress output if `g_config.verbose` is false for non-critical diagnostic messages.

### 5.7 Threading Rules

The threading model is described in `architecture.md`. These rules encode the invariants that make it correct:

**Always acquire locks in a consistent global order.** The server holds exactly one lock at a time: `m_clients_mutex`. The client holds exactly one lock at a time: `m_peers_mutex`. If a future change requires holding two locks simultaneously, document the order explicitly and add a comment explaining why it is safe.

**Never call a blocking I/O function while holding a mutex.** `net_send_raw`, `net_recv_raw`, `net_send_packet`, `net_recv_packet`, `sock_close` — all of these can block. Copy the socket handle and any data you need out from under the lock, release the lock, then do the I/O.

```cpp
// Good — lock released before I/O
{
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    auto it = m_clients.find(id);
    if (it == m_clients.end()) return false;
    target_sock = it->second->sock;     // copy handle
}
net_send_packet(target_sock, ...);      // I/O outside lock

// Bad — I/O inside lock
{
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    net_send_packet(m_clients[id]->sock, ...);    // DEADLOCK RISK
}
```

**Use `std::atomic<bool>` for stop/alive flags.** Do not use a plain `bool` protected by a mutex for flags that are read frequently by one thread and written occasionally by another. The overhead of a `mutex` acquisition on every poll is unnecessary; `std::atomic<bool>` with default sequential-consistency ordering is correct and cheap.

**`detach()` immediately after `thread.launch`.** Client handler threads are detached so the accept loop never blocks. Detached threads must not access any state whose lifetime they cannot guarantee; they should only access `ClientInfo` via the `ClientInfo*` they own, and they must set `ci->alive = false` and return before the `ClientInfo` is destroyed.

### 5.8 Memory Management

Use automatic storage whenever the lifetime is bounded to a scope. Use `std::vector`, `std::string`, and `std::map` instead of raw heap allocation. If you must allocate with `new`, the owning object must delete it in its destructor and all exception-exit paths (or use a smart pointer if you can do so without C++14 features — i.e., `std::unique_ptr` is fine, `std::make_unique` is not).

The only raw `new` expressions in the codebase are `new ClientInfo` in the server accept loop. These are intentional and justified by the detached-thread ownership model. Do not add more.

Do not use `malloc`/`free` except inside the platform socket layer where POSIX APIs return C-allocated data.

### 5.9 Platform Abstraction

The `network.hpp` / `network.cpp` module exists specifically to contain all platform differences. The abstractions provided are:

| Abstraction | What it hides |
|---|---|
| `sock_t` | `SOCKET` (Win32) vs `int` (POSIX) |
| `INVALID_SOCK` | `INVALID_SOCKET` vs `-1` |
| `sock_close(s)` | `closesocket(s)` vs `close(s)` |
| `net_init()` / `net_cleanup()` | `WSAStartup` / `WSACleanup` vs no-ops |
| `net_error_str()` | `WSAGetLastError` vs `errno`/`strerror` |
| `net_set_nonblocking(s, b)` | `ioctlsocket` vs `fcntl(F_SETFL)` |

All code outside `network.cpp` uses these abstractions exclusively. If you find yourself writing `#ifdef _WIN32` anywhere else in the codebase, move the platform difference into `network.cpp` instead.

### 5.10 Formatting and Layout

- Indentation: 4 spaces. No tabs.
- Line length: soft limit 100 characters, hard limit 120.
- Brace style: Allman (opening brace on its own line) for class/function/namespace bodies; K&R (opening brace on the same line) is also acceptable as long as a file is internally consistent.
- Spaces: one space after `if`, `for`, `while`, `switch`; no space between a function name and its argument list; one space around binary operators.
- Blank lines: one blank line between method definitions within a class body; two blank lines between top-level definitions in a `.cpp` file.
- Trailing whitespace: none. Most editors can strip this on save.
- End-of-file newline: one newline at the end of every file.

Do not run an auto-formatter (clang-format, astyle) across files you did not author. Mass reformatting creates noise in `git blame` and makes bisecting regressions harder.

---

## 6. Documentation Standards

SafeSocket uses **Doxygen-style block comments** for all API-facing declarations (anything in a `.hpp` file) and for non-obvious implementation details in `.cpp` files. The project does not currently generate Doxygen HTML, but the comment format is maintained so that generation can be enabled at any time without a documentation pass.

### 6.1 File Headers

Every source file (`*.cpp`, `*.hpp`) must begin with a Doxygen file-level comment:

```cpp
/**
 * @file network.cpp
 * @brief Platform socket abstraction layer for SafeSocket.
 *
 * Provides a thin, consistent API over BSD sockets (POSIX) and Winsock2
 * (Windows). All platform-specific `#ifdef` blocks live here; the rest of
 * the codebase calls only the `net_*` functions declared in network.hpp.
 *
 * @author  SafeSocket Project
 * @version 1.0
 */
```

Required tags: `@file`, `@brief`. Strongly recommended: `@author`, `@version`.

The `@brief` line is a single sentence that describes what the file contains or implements — not what the file *is*. Write "Implements the RC4 stream cipher" not "rc4 implementation file".

If the file has notable design decisions, constraints, or non-obvious dependencies, add them as additional paragraphs after the brief. See `crypto.cpp` for an example that documents algorithm-level security notes.

### 6.2 Class Documentation

Class block comments appear immediately before the `class` keyword in the header file:

```cpp
/**
 * @brief TCP chat-and-file-transfer server managing multiple concurrent clients.
 *
 * The server spawns one thread per connected client to handle receive loops,
 * one accept thread to listen for incoming connections, and an optional
 * keepalive thread to send periodic pings. All access to the client map is
 * protected by @c m_clients_mutex.
 *
 * ### Thread Safety
 * Public methods that touch the client map acquire @c m_clients_mutex
 * internally. Do not call public methods from within a context that already
 * holds this lock.
 *
 * ### Typical Lifecycle
 * @code
 * Server server;
 * if (server.start("0.0.0.0", 9000)) {
 *     server.run_cli();   // blocks until the operator types /quit
 * }
 * @endcode
 */
class Server {
```

The class comment must cover:
- **One-sentence brief** describing the core responsibility.
- **Longer description** (if needed) explaining internal structure.
- **Thread Safety section** for any class that contains mutexes or atomic flags.
- **Typical Lifecycle** using `@code` / `@endcode` if the construction/use/destruction sequence is non-obvious.

Do not duplicate the header comment in the `.cpp` file. The `.cpp` file comment describes the *implementation*, not the interface.

### 6.3 Struct and Field Documentation

Plain structs used as data carriers (like `ClientInfo`, `PeerInfo`, `PacketHeader`) document every field inline using the `///<` trailing-doc form:

```cpp
struct ClientInfo {
    uint32_t    id;            ///< Unique numeric identifier assigned by the server on connection.
    sock_t      sock;          ///< Connected socket handle used for all I/O with this client.
    std::string ip;            ///< Dotted-decimal IPv4 address of the remote endpoint.
    std::string nickname;      ///< Display name supplied by the client; defaults to "client_<id>".
    std::time_t connected_at;  ///< UNIX timestamp of the moment the connection was accepted.
    bool        authenticated; ///< true once the client has supplied a valid access key.
    std::atomic<bool> alive;   ///< Atomic liveness flag; set to false to request thread shutdown.
```

The `///<` comment must fit on one line. If the field requires more explanation — for example, a constraint, an invariant, or a relationship to another field — use a full `/** ... */` block directly above the field:

```cpp
    /**
     * @brief Thread running Server::client_loop() for this client.
     *
     * Detached immediately after construction so the accept loop can continue
     * without waiting for it. The thread must not outlive the ClientInfo object
     * it was given a pointer to.
     */
    std::thread thread;
```

For protocol structs (`PacketHeader`, `FileStartPayload`), document the field using its wire-level meaning, not its C++ type:

```cpp
struct PacketHeader {
    uint32_t magic;      ///< Protocol magic number; must equal MAGIC (0x534B5A4F).
    uint32_t msg_type;   ///< One of the MsgType values identifying the packet's purpose.
    uint32_t sender_id;  ///< Numeric ID of the originating client, or SERVER_ID.
    uint32_t target_id;  ///< Numeric ID of the intended recipient, or BROADCAST_ID for broadcasts.
    uint32_t data_len;   ///< Length in bytes of the payload that immediately follows this header.
```

### 6.4 Function Documentation

Every public function and every non-trivial private function gets a Doxygen block in the header (for declarations) or immediately before the definition in the `.cpp` (for static helper functions):

```cpp
/**
 * @brief Apply repeating-key XOR to a byte buffer in-place.
 *
 * Each byte at index @c i is XOR-ed with @c key[i % klen], where @c klen is
 * the length of the key. The operation is its own inverse, so this function
 * serves as both encrypt and decrypt.
 *
 * If @p key is empty the function returns immediately, leaving @p buf
 * unchanged.
 *
 * @param[in,out] buf  Buffer to transform in-place.
 * @param[in]     key  Repeating XOR key. If empty, buf is not modified.
 */
static void xor_crypt(std::vector<uint8_t>& buf, const std::string& key);
```

Required sections and tags:

| Element | Required? | Notes |
|---|---|---|
| `@brief` | Always | One sentence, ends with period |
| Body paragraph | When non-trivial | Explain algorithm, edge cases, preconditions |
| `@param[in]` / `@param[out]` / `@param[in,out]` | When parameters exist | Direction qualifier is mandatory |
| `@return` | When return value is not `void` | Describe every meaningful return state |
| `@note` | When there is a platform caveat or threading constraint | |
| `@throws` | Never — SafeSocket does not throw | |

The `@brief` must be a complete sentence with a capital first letter and a terminal period. It describes what the function *does* (imperative mood or third-person present): "Sends a raw byte buffer over a socket", not "Raw send" or "Sends bytes".

Parameter descriptions must explain what the parameter *means*, not merely restate its type. `@param[in] key  Repeating XOR key; must not be empty for any transformation to occur` is useful. `@param[in] key  The key string` is not.

For simple one-liner helpers (e.g., a getter that returns a single member), a brief alone is sufficient:

```cpp
/**
 * @brief Returns the current number of connected clients.
 */
int client_count() const;
```

### 6.5 Inline Comments

Inline comments (`//`) explain the *why*, not the *what*. If the code is clear about what it does, a comment that just restates it is noise.

```cpp
// Bad — restates the code
s1 = (s1 + c) % 0xFFFF;   // add c to s1 and take modulo

// Good — explains the why
s1 = (s1 + c) % 0xFFFF;   // Fletcher-32: keep accumulator in 16-bit range

// Good — explains a non-obvious constraint
sock_close(ci->sock);
ci->sock = INVALID_SOCK;   // prevent double-close if disconnect() races with recv_loop()

// Good — explains a deliberate deviation from the obvious approach
// +256 before masking keeps the subtraction non-negative on platforms
// where char is signed and the shift is larger than the data byte.
buf[i] = static_cast<uint8_t>((buf[i] - shift + 256) & 0xFF);
```

Comments must be grammatically correct sentences or very short imperative fragments ("Drain remaining data before close"). Avoid abbreviations unless they are standard in the domain (`RC4`, `KSA`, `PRGA`, `POSIX`, `TCP`).

Comment placement: end-of-line comments are acceptable for short annotations. For anything requiring more than ~60 characters, place the comment on its own line above the code it describes.

Do not leave commented-out code in a PR. Use version control. If you are unsure whether a block of code is still needed, note it in the PR description, not in the diff.

### 6.6 Section Dividers

Source files use a consistent visual structure with `//` dividers to break up logically distinct sections. Use the exact format already established in the codebase:

```cpp
// ============================================================
//  XOR Cipher
// ============================================================
```

Sixty-equals signs, two spaces, the section name, two spaces, sixty equals signs — on three lines. This is the section-level divider. Use it to separate major groups of related functions within a `.cpp` file (e.g., "Name → Enum Conversion", "XOR Cipher", "Vigenère Cipher", "RC4 Stream Cipher").

Within a class definition in a header, use the shorter form:

```cpp
    // --------------------------------------------------------
    //  Constructor / Destructor
    // --------------------------------------------------------
```

Do not invent other divider styles.

### 6.7 What Not to Comment

The following do not need comments:

- `#pragma once` — its purpose is universal knowledge
- Simple getters and setters where the name is self-documenting
- Closing braces of short functions — only add `// namespace foo` for closing a namespace
- `#include` directives — if you find yourself writing `// for std::vector`, the include is probably in the wrong place

---

## 7. Protocol Changes

The wire protocol is defined entirely in `protocol.hpp`. Changing it is a **breaking change** that makes the new binary incompatible with any older binary it might communicate with. Protocol changes require:

1. A `@note Breaking change: incompatible with pre-vX.Y clients.` in the Doxygen block of the affected struct or constant.
2. An update to the `MAGIC` constant if the header layout changes (increment the minor version component embedded in the ASCII bytes).
3. A migration note in the PR description.
4. New `MsgType` values appended at the end of the enum — never renumbering existing values.
5. New test cases in `tests/unit/protocol/`.

`PacketHeader` is `#pragma pack(push, 1)` and must remain exactly 20 bytes. Any change to the header size is a protocol version bump.

When adding a new `MsgType`, document its payload format in the enum comment using the same bracket notation as existing types:

```cpp
MSG_NICK_SET = 12,  ///< Client requests a nickname change.
                    ///< Payload: [uint16_t nick_len][nick bytes (not null-terminated)].
```

---

## 8. Crypto Module Rules

`crypto.hpp` / `crypto.cpp` implement XOR, Vigenère, and RC4. These algorithms are weak by modern standards and are documented as such. The following rules prevent them from being misused or silently broken:

**All three algorithms operate in-place on `std::vector<uint8_t>`** via `crypto_encrypt` and `crypto_decrypt`. The function signatures must not change. Do not add overloads that operate on raw pointers — callers must construct a `vector`.

**Empty key is always a no-op**, regardless of algorithm. This is a documented invariant and there are test cases for it. Any algorithm addition must preserve this behaviour.

**RC4 state is created fresh for each call.** There is no persistent `RC4State` stored between calls; the state lives on the stack for the duration of `rc4_crypt`. This is intentional: packet encryption in SafeSocket is stateless. Do not change this to a streaming model without a protocol version bump and a full test pass.

**Do not add stronger algorithms** (AES, ChaCha20, etc.) without updating `EncryptType`, `encrypt_type_name()`, `encrypt_type_from_string()`, the `Config::set("encrypt_type", ...)` handler, the usage docs, and the architecture docs simultaneously. A partial addition — algorithm implemented but not wired into the config — will be rejected.

**Known test vectors must be preserved.** `test_rc4.cpp` contains a test against the RFC 6229 keystream for key `"Key"`. If you change the RC4 implementation, this test proves it. Do not weaken or remove it.

---

## 9. Network Layer Rules

`network.hpp` exports exactly the functions listed in the architecture documentation. The interface is deliberately minimal. Do not add functions to `network.hpp` unless the alternative is duplicating platform `#ifdef` logic in two or more callers.

**`net_send_packet` and `net_recv_packet` are the only functions that know about the wire format.** They encrypt/decrypt the payload and frame it with a `PacketHeader`. Everything above this layer deals in `msg_type`, `sender_id`, `target_id`, and `payload bytes` — never in raw `PacketHeader` bytes.

**`net_listen(host, port, backlog)` always calls `net_set_reuseaddr` internally** before `bind`. Do not add a parameter to skip this; `SO_REUSEADDR` is always the right choice for a server that may restart quickly.

**`net_connect(host, port, timeout_sec)` uses a non-blocking connect with `select` for timeout.** The timeout parameter is in whole seconds. Do not change it to milliseconds without updating every call site and the usage documentation.

**`INVALID_SOCK` must be checked after every `net_listen`, `net_connect`, and `net_accept` call.** Never assume these succeed. The test suite verifies that the network layer returns `INVALID_SOCK` on failure — maintain this contract.

---

## 10. Config System Rules

`Config` is a plain struct with 24 named fields. `Config::set(key, value)` and `Config::get(key)` provide string-based dynamic access for CLI and file I/O. The global singleton `g_config` is shared across the entire process.

**All 24 fields must have a sensible default** set in the default constructor. The defaults are documented in `usage.md`. If you add a field, add a default and update `usage.md` simultaneously.

**Sensitive fields (`encrypt_key`, `access_key`) must be masked in `Config::get()`** — return `"***hidden***"` instead of the actual value. Add the field name to the sensitive-field check list in `config.cpp`. New password-like fields must do the same.

**`Config::set()` must handle invalid values gracefully.** An invalid integer for `port` (e.g., `"not_a_number"`) must log a warning and leave the field unchanged; it must never throw, crash, or produce undefined behaviour. Use `try/catch` around `std::stoi` and similar.

**Key names are case-insensitive.** `Config::set()` uppercases the key before the comparison. Every new key must be checked against the uppercased form.

**Do not access `g_config` from a thread without ensuring that the write it depends on has already completed.** Config is set up once before `server.start()` or `client.connect()` is called and is treated as effectively read-only after that point. If you add a config key that can be changed at runtime via `/set`, document the thread-safety implications.

---

## 11. Testing Requirements

Every PR that changes behaviour (not just docs or formatting) must include tests. Use the test framework in `tests/helpers/test_framework.hpp`. Do not add Google Test, Catch2, or any other framework.

**Unit tests** are required for:
- Any new free function in `crypto.hpp`, `network.hpp`, `config.hpp`, or `protocol.hpp`
- Any change to existing algorithm logic (even a one-line fix)
- Any new `Config` key

**Integration tests** are required for:
- Any new client or server command
- Any new `MsgType`
- Any change to connection lifecycle (connect, auth, keepalive, disconnect)

**Stress tests** should be added for:
- Any change to the threading model
- Any change to the keepalive or disconnect logic

For every bug fix, add a regression test that fails on the unfixed code and passes on the fixed code. Name it to describe the bug, not the fix:

```
// Regression for: server deadlock when client disconnects during broadcast
TEST(StressClients, DisconnectDuringBroadcastNoDeadlock) { ... }
```

Run the full test suite plus sanitisers before marking a PR ready for review:

```bash
cd tests && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSS_ASAN=ON -DSS_TSAN=ON
cmake --build . --parallel
ctest --output-on-failure
```

A PR that breaks existing tests will not be reviewed until the tests pass.

---

## 12. Pull Request Checklist

Before requesting review, confirm every item:

**Code quality**
- [ ] Compiles on Linux with `g++ -std=c++11 -Wall -Wextra -Werror -pthread`
- [ ] Compiles on Windows with `cl /std:c++17 /W3 /EHsc ... ws2_32.lib`
- [ ] No new `#ifdef _WIN32` outside `network.cpp`
- [ ] No external library includes added
- [ ] No C++14 or C++17 features used
- [ ] All new public APIs have Doxygen block comments in the header

**Documentation**
- [ ] Every new `.cpp` / `.hpp` file has a `@file` / `@brief` header
- [ ] Every new class has a Doxygen class comment with Thread Safety section if applicable
- [ ] Every new struct has `///<` trailing comments on all fields
- [ ] Every new public function has `@param` and `@return` tags
- [ ] Inline comments explain *why*, not *what*
- [ ] No commented-out code left in the diff
- [ ] If the change affects `usage.md`, `build.md`, or `architecture.md`, those files are updated in the same PR

**Testing**
- [ ] New unit tests added for new or changed logic
- [ ] All existing tests pass with `-DSS_ASAN=ON`
- [ ] A clean `-DSS_TSAN=ON` run if threading code was touched
- [ ] No test uses an external test framework

**Protocol / breaking changes**
- [ ] `protocol.hpp` `PacketHeader` is still exactly 20 bytes (if touched)
- [ ] No existing `MsgType` value was renumbered
- [ ] `MAGIC` was updated if the header layout changed
- [ ] Breaking change is noted in `@note` in `protocol.hpp` and in the PR description

---

## 13. Reporting Bugs

Open an issue with the following information. Incomplete reports may be closed without response.

**Title format:** `[component] Brief description` — e.g., `[crypto] RC4 produces wrong output when key contains null byte`

**Required fields:**

1. **SafeSocket version** — output of `safesocket --version` or the git commit hash
2. **Operating system and compiler** — e.g., `Ubuntu 22.04, GCC 11.3`
3. **Steps to reproduce** — the exact commands you ran, including config file contents if relevant
4. **Expected behaviour** — what should have happened
5. **Actual behaviour** — what happened instead, including the full error output
6. **Minimal reproducer** — a config file, input sequence, or test case that demonstrates the bug in as few steps as possible
