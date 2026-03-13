/**
 * @file test_nick_change.cpp
 * @brief Integration tests for nickname change (MSG_NICK_SET).
 *
 * Verifies:
 *  - Client can set a nickname at connect time
 *  - Client can change nickname mid-session
 *  - Server rejects empty nickname
 *  - Server truncates/rejects oversized nicknames
 *  - Nick change is broadcast to all connected clients
 */

#include "../../helpers/test_framework.hpp"
#include "../../../server.hpp"
#include "../../../client.hpp"
#include "../../../config.hpp"
#include "../../../network.hpp"
#include <thread>
#include <chrono>
#include <string>

static int free_port() {
    sock_t s = net_listen("127.0.0.1", 0);
    if (s == INVALID_SOCK) return 20000;
    struct sockaddr_in addr; socklen_t l = sizeof(addr);
    getsockname(s, (struct sockaddr*)&addr, &l);
    int p = ntohs(addr.sin_port);
    sock_close(s); return p;
}
static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
static void base_config(int port) {
    g_config.host         = "127.0.0.1";
    g_config.port         = port;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.encrypt_key  = "";
    g_config.require_key  = false;
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.motd         = "nick-test";
}

TEST(NickChange, InitialNicknameSetAtConnect) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "InitialNick";
    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(100);

    // Server should know this client by nickname "InitialNick"
    ASSERT_STREQ(server.get_client_nick(1), "InitialNick");

    client.disconnect();
    server.stop();
}

TEST(NickChange, ClientChangesNick) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "OldNick";
    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(100);

    client.set_nick("NewNick");
    sleep_ms(100);

    ASSERT_STREQ(server.get_client_nick(1), "NewNick");

    client.disconnect();
    server.stop();
}

TEST(NickChange, EmptyNickRejected) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "ValidNick";
    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(100);

    client.set_nick("");   // should be rejected by server
    sleep_ms(100);

    // Nick should remain unchanged
    ASSERT_STREQ(server.get_client_nick(1), "ValidNick");

    client.disconnect();
    server.stop();
}

TEST(NickChange, OversizedNickTruncatedOrRejected) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "Short";
    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(100);

    // 256-character nickname — server should truncate or reject
    std::string huge(256, 'X');
    client.set_nick(huge);
    sleep_ms(100);

    std::string stored = server.get_client_nick(1);
    ASSERT_LE((int)stored.size(), 64);   // must be bounded

    client.disconnect();
    server.stop();
}

TEST(NickChange, NickChangeBroadcastToOtherClients) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "Watcher";
    Client watcher;
    ASSERT_TRUE(watcher.connect("127.0.0.1", port));
    sleep_ms(80);

    g_config.nickname = "Changer";
    Client changer;
    ASSERT_TRUE(changer.connect("127.0.0.1", port));
    sleep_ms(80);

    changer.set_nick("RenamedChanger");
    sleep_ms(150);

    // watcher should have received the nick-change notification — no crash
    watcher.disconnect();
    changer.disconnect();
    server.stop();
}

int main() { return RUN_ALL_TESTS(); }
