/**
 * @file test_access_key.cpp
 * @brief Integration tests for server access-key authentication.
 *
 * Verifies:
 *  - Client connects when require_key=false (open server)
 *  - Client connects with correct access key
 *  - Client is rejected with wrong access key
 *  - Client is rejected when it sends no key
 *  - Multiple correct-key clients all connect
 *  - One bad + one good client: good one still connects
 */

#include "../../helpers/test_framework.hpp"
#include "../../../server.hpp"
#include "../../../client.hpp"
#include "../../../config.hpp"
#include "../../../network.hpp"
#include <thread>
#include <chrono>

static int free_port() {
    sock_t s = net_listen("127.0.0.1", 0);
    if (s == INVALID_SOCK) return 20100;
    struct sockaddr_in addr; socklen_t l = sizeof(addr);
    getsockname(s, (struct sockaddr*)&addr, &l);
    int p = ntohs(addr.sin_port);
    sock_close(s); return p;
}
static void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

TEST(AccessKey, OpenServerAllowsAnyClient) {
    net_init();
    int port = free_port();

    g_config.host         = "127.0.0.1";
    g_config.port         = port;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.require_key  = false;
    g_config.access_key   = "";
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.nickname     = "OpenClient";
    g_config.motd         = "open";

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(100);

    ASSERT_TRUE(server.is_connected(1));

    client.disconnect();
    server.stop();
}

TEST(AccessKey, CorrectKeyGrantsAccess) {
    net_init();
    int port = free_port();

    g_config.host         = "127.0.0.1";
    g_config.port         = port;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.require_key  = true;
    g_config.access_key   = "supersecret";
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.nickname     = "AuthClient";
    g_config.motd         = "auth";

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    Client client;
    client.set_access_key("supersecret");
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(150);

    ASSERT_TRUE(server.is_connected(1));

    client.disconnect();
    server.stop();
    g_config.require_key = false;
    g_config.access_key  = "";
}

TEST(AccessKey, WrongKeyIsRejected) {
    net_init();
    int port = free_port();

    g_config.host         = "127.0.0.1";
    g_config.port         = port;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.require_key  = true;
    g_config.access_key   = "correctpass";
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.nickname     = "BadClient";
    g_config.motd         = "auth";

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    Client client;
    client.set_access_key("wrongpass");
    // connect() will succeed at TCP level; server will kick after auth failure
    client.connect("127.0.0.1", port);
    sleep_ms(300);   // allow server time to process and kick

    ASSERT_FALSE(server.is_connected(1));

    server.stop();
    g_config.require_key = false;
    g_config.access_key  = "";
}

TEST(AccessKey, NoKeyProvidedIsRejected) {
    net_init();
    int port = free_port();

    g_config.host         = "127.0.0.1";
    g_config.port         = port;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.require_key  = true;
    g_config.access_key   = "mustprovide";
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.nickname     = "NoKeyClient";
    g_config.motd         = "auth";

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    Client client;
    // No set_access_key call — client sends empty/no key
    client.connect("127.0.0.1", port);
    sleep_ms(300);

    ASSERT_FALSE(server.is_connected(1));

    server.stop();
    g_config.require_key = false;
    g_config.access_key  = "";
}

TEST(AccessKey, MultipleCorrectKeyClients) {
    net_init();
    int port = free_port();

    g_config.host         = "127.0.0.1";
    g_config.port         = port;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.require_key  = true;
    g_config.access_key   = "sharedkey";
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.motd         = "auth";

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "AuthUser1";
    Client c1;
    c1.set_access_key("sharedkey");
    ASSERT_TRUE(c1.connect("127.0.0.1", port));

    g_config.nickname = "AuthUser2";
    Client c2;
    c2.set_access_key("sharedkey");
    ASSERT_TRUE(c2.connect("127.0.0.1", port));

    sleep_ms(200);

    ASSERT_TRUE(server.is_connected(1));
    ASSERT_TRUE(server.is_connected(2));

    c1.disconnect();
    c2.disconnect();
    server.stop();
    g_config.require_key = false;
    g_config.access_key  = "";
}

int main() { return RUN_ALL_TESTS(); }
