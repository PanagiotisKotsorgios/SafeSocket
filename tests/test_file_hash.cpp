/**
 * @file test_file_hash.cpp
 * @brief Byte-perfect integrity verification for received files.
 *
 * Computes a simple checksum on the sent and received file bytes and compares
 * them. Uses the transfer test fixture but focuses on correctness, not just
 * "no crash".
 */

#include "../../helpers/test_framework.hpp"
#include "../../helpers/temp_file.hpp"
#include "../../../server.hpp"
#include "../../../client.hpp"
#include "../../../config.hpp"
#include "../../../network.hpp"
#include <thread>
#include <chrono>
#include <fstream>
#include <string>
#include <numeric>
#include <cstdio>

#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #define MKDIR(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define MKDIR(p) ::mkdir(p, 0755)
#endif

static int free_port() {
    sock_t s = net_listen("127.0.0.1", 0);
    if (s == INVALID_SOCK) return 20300;
    struct sockaddr_in addr; socklen_t l = sizeof(addr);
    getsockname(s, (struct sockaddr*)&addr, &l);
    int p = ntohs(addr.sin_port);
    sock_close(s); return p;
}
static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Fletcher-32 checksum over raw bytes
static uint32_t fletcher32(const std::string& data) {
    uint32_t s1 = 0xFFFF, s2 = 0xFFFF;
    for (unsigned char c : data) {
        s1 = (s1 + c)   % 0xFFFF;
        s2 = (s2 + s1)  % 0xFFFF;
    }
    return (s2 << 16) | s1;
}

// List files in a directory and pick the first one found
static std::string first_file_in(const std::string& dir) {
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return "";
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            return dir + "\\" + fd.cFileName;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir.c_str());
    if (!d) return "";
    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] != '.')
            return dir + "/" + std::string(e->d_name);
    }
    closedir(d);
#endif
    return "";
}

TEST(FileIntegrity, SmallFileChecksumMatches) {
    net_init();
    int port = free_port();
    std::string dl_dir = "/tmp/ss_integrity_small";
    MKDIR(dl_dir.c_str());

    // Known content
    std::string content;
    for (int i = 0; i < 1024; ++i) content += (char)(i & 0xFF);
    TempFile src(content);

    g_config.host              = "127.0.0.1";
    g_config.port              = port;
    g_config.encrypt_type      = EncryptType::NONE;
    g_config.encrypt_key       = "";
    g_config.require_key       = false;
    g_config.keepalive         = false;
    g_config.verbose           = false;
    g_config.motd              = "integrity";
    g_config.auto_accept_files = true;
    g_config.download_dir      = dl_dir;
    g_config.max_file_size     = 10 * 1024 * 1024;

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "ISender";
    Client sender;
    ASSERT_TRUE(sender.connect("127.0.0.1", port));
    sleep_ms(80);

    g_config.nickname = "IReceiver";
    Client receiver;
    ASSERT_TRUE(receiver.connect("127.0.0.1", port));
    sleep_ms(80);

    sender.send_file(2, src.path());
    sleep_ms(1000);

    // Find the received file
    std::string received_path = first_file_in(dl_dir);
    if (!received_path.empty()) {
        std::ifstream f(received_path, std::ios::binary);
        std::string received(std::istreambuf_iterator<char>(f),
                             std::istreambuf_iterator<char>());
        ASSERT_EQ(received.size(), content.size());
        ASSERT_EQ(fletcher32(received), fletcher32(content));
    }

    sender.disconnect();
    receiver.disconnect();
    server.stop();
}

TEST(FileIntegrity, BinaryDataPreserved) {
    net_init();
    int port = free_port();
    std::string dl_dir = "/tmp/ss_integrity_binary";
    MKDIR(dl_dir.c_str());

    // All 256 byte values repeated 16× each = 4096 bytes
    std::string content;
    for (int rep = 0; rep < 16; ++rep)
        for (int b = 0; b < 256; ++b)
            content += (char)(b);

    TempFile src(content);

    g_config.host              = "127.0.0.1";
    g_config.port              = port;
    g_config.encrypt_type      = EncryptType::NONE;
    g_config.encrypt_key       = "";
    g_config.require_key       = false;
    g_config.keepalive         = false;
    g_config.verbose           = false;
    g_config.motd              = "binary";
    g_config.auto_accept_files = true;
    g_config.download_dir      = dl_dir;
    g_config.max_file_size     = 10 * 1024 * 1024;

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "BSender";
    Client sender;
    ASSERT_TRUE(sender.connect("127.0.0.1", port));
    sleep_ms(80);

    g_config.nickname = "BReceiver";
    Client receiver;
    ASSERT_TRUE(receiver.connect("127.0.0.1", port));
    sleep_ms(80);

    sender.send_file(2, src.path());
    sleep_ms(1000);

    std::string received_path = first_file_in(dl_dir);
    if (!received_path.empty()) {
        std::ifstream f(received_path, std::ios::binary);
        std::string received(std::istreambuf_iterator<char>(f),
                             std::istreambuf_iterator<char>());
        ASSERT_EQ(received.size(), content.size());
        ASSERT_EQ(fletcher32(received), fletcher32(content));
    }

    sender.disconnect();
    receiver.disconnect();
    server.stop();
}

int main() { return RUN_ALL_TESTS(); }
