# SafeSocketGUI — Build Guide (Windows MSYS2 UCRT64)

## Prerequisites

Open **MSYS2 UCRT64** shell and install dependencies:

```bash
pacman -Syu

pacman -S \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-SDL3 \
  mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-pkg-config \
  make
```

## Build

```bash
cd /path/to/gui_final          # cd to this folder
make -f Makefile.win -j$(nproc)
```

Output: `SafeSocketGUI.exe`

## Run

Double-click `SafeSocketGUI.exe`, or from MSYS2 shell:

```bash
./SafeSocketGUI.exe
```

The DLLs needed at runtime (SDL3, OpenSSL, etc.) are in:
`C:\msys64\ucrt64\bin\`

To run outside MSYS2, copy the following DLLs next to the exe:
```
SDL3.dll
libssl-3-x64.dll
libcrypto-3-x64.dll
libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll
```

Or run this convenience command to copy them automatically:
```bash
cp $(ldd SafeSocketGUI.exe | grep ucrt64 | awk '{print $3}') .
```

## Features implemented

| Feature                        | Status |
|-------------------------------|--------|
| Connect as client              | ✅ Full |
| Start server                   | ✅ Full |
| Broadcast chat                 | ✅ Full |
| Private messaging (PM)         | ✅ Full |
| Online users sidebar           | ✅ Full |
| Nickname change                | ✅ Full |
| File send → server             | ✅ Full |
| File send → client             | ✅ Full |
| File transfer history          | ✅ Full |
| Server admin panel             | ✅ Full |
| Kick clients from server       | ✅ Full |
| Server broadcast (admin msg)   | ✅ Full |
| Auto-reconnect                 | ✅ Full |
| Encryption (XOR/Vigenere/RC4)  | ✅ Full |
| Access key auth                | ✅ Full |
| Profile save/load              | ✅ Full |
| Dark/Light theme               | ✅ Full |
| Settings panel                 | ✅ Full |
| Log to file                    | ✅ Full |
| AES-256 app security lock      | ✅ Full |
| Connection log                 | ✅ Full |
| Keepalive ping                 | ✅ Full |

## Clean

```bash
make -f Makefile.win clean
```
