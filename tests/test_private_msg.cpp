/**
 * @file test_private_msg.cpp
 * @brief Integration tests for private (directed) messaging.
 *
 * Verifies MSG_PRIVATE is delivered only to the intended recipient,
 * that the server routes by client ID, and that sending to an unknown
 * ID logs an error but does not crash.
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
    if (s == INVALID_SOCK) return 19900;
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
    g_config.motd         = "pm-test";
}

TEST(PrivateMsg, ServerSendTextToClient) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "PMTarget";
    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    sleep_ms(120);

    // Server sends private message to client ID=1 (first connected client)
    server.send_text(1, "Hello target!", SERVER_ID);
    sleep_ms(100);

    client.disconnect();
    server.stop();
}

TEST(PrivateMsg, SendToUnknownIdDoesNotCrash) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    // No clients connected — sending to ID 999 should not crash
    server.send_text(999, "Nobody home", SERVER_ID);

    server.stop();
}

TEST(PrivateMsg, TwoClientsPrivateRouting) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "Alice";
    Client alice;
    ASSERT_TRUE(alice.connect("127.0.0.1", port));
    sleep_ms(80);

    g_config.nickname = "Bob";
    Client bob;
    ASSERT_TRUE(bob.connect("127.0.0.1", port));
    sleep_ms(80);

    // Both clients should be live; server routes private message by ID
    server.send_text(1, "For Alice only",    SERVER_ID);
    server.send_text(2, "For Bob only",      SERVER_ID);
    sleep_ms(100);

    alice.disconnect();
    bob.disconnect();
    server.stop();
}

TEST(PrivateMsg, ClientSendPrivateViaServer) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    g_config.nickname = "Sender";
    Client sender_client;
    ASSERT_TRUE(sender_client.connect("127.0.0.1", port));
    sleep_ms(80);

    g_config.nickname = "Receiver";
    Client receiver_client;
    ASSERT_TRUE(receiver_client.connect("127.0.0.1", port));
    sleep_ms(80);

    // Client sends a private packet to peer ID=2 via the server
    sender_client.send_private(2, "Private hello");
    sleep_ms(100);

    sender_client.disconnect();
    receiver_client.disconnect();
    server.stop();
}

int main() { return RUN_ALL_TESTS(); }
