# Security Policy

## Supported Versions

Only the most recent release receives security patches. SafeSocket is in early development — the version line is short and backwards-patching is not practical at this stage.

| Version | Supported |
|---|---|
| 0.0.1 | Yes — current release |
| < 0.0.1 | No |

This table is updated with every release. When 0.0.2 ships, support for 0.0.1 will be dropped unless a critical vulnerability requires a backport.

---

## Security Scope

The following components are considered in scope for security reports:

| Component | File(s) | Notes |
|---|---|---|
| Wire protocol | `protocol.hpp` | Header integrity, magic validation, oversized payload handling |
| Encryption layer | `crypto.cpp`, `crypto.hpp` | Cipher correctness, key handling, empty-key behaviour |
| Access key authentication | `server.cpp`, `config.cpp` | Auth bypass, timing side-channels, brute-force exposure |
| Network layer | `network.cpp`, `network.hpp` | Buffer overflows, socket misuse, connection exhaustion |
| Configuration parser | `config.cpp` | Injection via config file, sensitive field leakage |
| File transfer | `server.cpp`, `client.cpp` | Path traversal in filenames, oversized file handling |

The following are **out of scope** for this policy:

| Item | Reason |
|---|---|
| XOR, Vigenère, RC4 cipher weakness | Documented as obfuscation-grade in `usage.md`. Cryptographic weakness in these algorithms is a known limitation, not a vulnerability. |
| Denial of service via legitimate connection flooding | No rate-limiting exists in 0.0.1. This is a known gap tracked in the roadmap. |
| Security of data at rest | SafeSocket does not store messages or files beyond the active session. |

---

## Known Security Limitations in 0.0.1

These are documented design constraints, not undiscovered vulnerabilities. They are listed here for full transparency.

| Limitation | Detail | Tracked In |
|---|---|---|
| No authenticated encryption | XOR / Vigenère / RC4 provide obfuscation only. An attacker with network access can read traffic. | Roadmap 0.0.5 |
| No forward secrecy | The `encrypt_key` is a static pre-shared secret. Compromise of the key exposes all past sessions. | Roadmap 0.0.5 |
| No certificate-based identity | There is no way to verify the server's identity. Man-in-the-middle attacks are possible. | Roadmap 0.0.5 |
| `access_key` transmitted in `MSG_CONNECT` | The key is encrypted only if an `encrypt_type` other than `NONE` is configured. On an open connection it is sent in plaintext. | Roadmap 0.0.5 |
| No per-client rate limiting | A connected client can send an unbounded number of packets per second. | Roadmap 0.0.6 |
| File path not sanitised server-side | The receiving client writes to `download_dir/<filename>` where `<filename>` comes from the sender. Path traversal sequences in filenames are not currently stripped. | Roadmap 0.0.2 |

---

## Reporting a Vulnerability

**Do not open a public GitHub issue for security vulnerabilities.** Public disclosure before a fix is available puts all users at risk.

### How to Report

Send a report by email to the maintainer. Include the following:

| Field | Required | Description |
|---|---|---|
| SafeSocket version | Yes | Output of `safesocket --version` or the git commit hash |
| Operating system and compiler | Yes | e.g. Ubuntu 22.04, GCC 11.3 |
| Affected component | Yes | Which file or subsystem is affected |
| Description | Yes | Clear description of the vulnerability and what an attacker can achieve |
| Steps to reproduce | Yes | Exact commands, config files, or packet sequences that trigger the issue |
| Proof of concept | Strongly recommended | A minimal reproducer, test case, or packet capture |
| Suggested fix | Optional | If you have identified the root cause |

### What to Expect

| Timeframe | Action |
|---|---|
| Within 3 business days | Acknowledgement of receipt |
| Within 10 business days | Initial assessment — confirmed, needs more information, or out of scope |
| Within 30 business days | A fix or a documented workaround for confirmed vulnerabilities |
| Upon release | Credit in the release notes unless you request otherwise |

If a report requires more than 30 business days to resolve, you will receive a progress update every 10 business days until the fix ships.

### Disclosure Policy

SafeSocket follows **coordinated disclosure**:

1. Reporter submits the vulnerability privately.
2. Maintainer confirms and develops a fix.
3. A patched release is prepared and tested.
4. The fix ships and the release is tagged.
5. A public security advisory is published no later than 7 days after the release.

If no response is received within 10 business days of the initial report, the reporter is free to proceed with public disclosure at their discretion.

### Safe Harbour

Security research conducted in good faith and reported responsibly is welcomed. Testing should be performed against your own local instance of SafeSocket, not against any server you do not own or have explicit permission to test.
