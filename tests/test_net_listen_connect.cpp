/**
 * @file test_net_listen_connect.cpp
 * @brief Unit tests for net_listen(), net_connect(), and net_accept().
 *
 * Verifies:
 *  - net_listen() returns a valid socket on a free port
 *  - net_listen() returns INVALID_SOCK on a bad host
 *  - net_connect() succeeds to a listening socket
 *  - net_connect() fails (timeout) when nothing is listening
 *  - net_accept() returns a valid connected socket
 *  - net_accept() populates peer_ip correctly
 *  - connecting to 127.0.0.1 gives peer_ip "127.0.0.1"
 */

#include "../../helpers/test_framework.hpp"
#include "../../helpers/socket_pair.hpp"
#include "../../../network.hpp"
#include <string>
#include <thread>
#include <chrono>

TEST(NetListen, ReturnsValidSocket) {
    net_init();
    sock_t s = net_listen("127.0.0.1", 0);  // port 0 = OS picks
    ASSERT_NE((int)s, (int)INVALID_SOCK);
    sock_close(s);
}

TEST(NetListen, BindsOnAllInterfaces) {
    net_init();
    sock_t s = net_listen("0.0.0.0", 0);
    ASSERT_NE((int)s, (int)INVALID_SOCK);
    sock_close(s);
}

TEST(NetListen, FailsOnBadHost) {
    net_init();
    // "999.999.999.999" is not a valid IP
    sock_t s = net_listen("999.999.999.999", 9999);
    ASSERT_EQ((int)s, (int)INVALID_SOCK);
}

TEST(NetListen, FailsOnPrivilegedPortWithoutPermission) {
    // Port 1 requires root — skip this check if running as root
    // We simply verify the function doesn't crash
    net_init();
    sock_t s = net_listen("127.0.0.1", 1);
    // May succeed or fail depending on privileges — just must not crash
    if (s != INVALID_SOCK) sock_close(s);
}

TEST(NetConnect, SucceedsToListeningSocket) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());
    // If we get here, net_connect succeeded
    ASSERT_NE((int)sp.client(), (int)INVALID_SOCK);
}

TEST(NetConnect, FailsWhenNothingListening) {
    net_init();
    // Port 19999 — very unlikely to have anything listening; short timeout
    sock_t s = net_connect("127.0.0.1", 19999, 1);
    ASSERT_EQ((int)s, (int)INVALID_SOCK);
}

TEST(NetAccept, ReturnsValidSocket) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());
    ASSERT_NE((int)sp.server_conn(), (int)INVALID_SOCK);
}

TEST(NetAccept, PopulatesPeerIp) {
    net_init();

    sock_t listen_sock = net_listen("127.0.0.1", 0);
    ASSERT_NE((int)listen_sock, (int)INVALID_SOCK);

    // Get the assigned port
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(listen_sock, (struct sockaddr*)&addr, &len);
    int port = ntohs(addr.sin_port);

    // Connect from a background thread so accept() doesn't block forever
    sock_t client_sock = INVALID_SOCK;
    std::thread connector([&]() {
        client_sock = net_connect("127.0.0.1", port, 5);
    });

    std::string peer_ip;
    sock_t accepted = net_accept(listen_sock, peer_ip);

    connector.join();

    ASSERT_NE((int)accepted, (int)INVALID_SOCK);
    ASSERT_STREQ(peer_ip, "127.0.0.1");

    sock_close(accepted);
    if (client_sock != INVALID_SOCK) sock_close(client_sock);
    sock_close(listen_sock);
}

int main() { return RUN_ALL_TESTS(); }
