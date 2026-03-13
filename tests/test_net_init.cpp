/**
 * @file test_net_init.cpp
 * @brief Unit tests for net_init(), net_cleanup(), and socket option setters.
 *
 * Verifies:
 *  - net_init() returns true
 *  - net_cleanup() does not crash
 *  - net_init() is idempotent (safe to call twice)
 *  - net_set_nonblocking() on a valid socket
 *  - net_set_nodelay() on a valid socket
 *  - net_set_reuseaddr() on a valid socket
 *  - socket option setters return false on INVALID_SOCK
 *  - net_error_str() returns a non-empty string after an error
 */

#include "../../helpers/test_framework.hpp"
#include "../../../network.hpp"
#include <string>

TEST(NetInit, ReturnsTrueOnFirstCall) {
    ASSERT_TRUE(net_init());
}

TEST(NetInit, CleanupDoesNotCrash) {
    net_init();
    net_cleanup();
    // Re-init for subsequent tests
    net_init();
}

TEST(NetInit, IdempotentSecondCall) {
    ASSERT_TRUE(net_init());
    ASSERT_TRUE(net_init());   // second call must also succeed
}

TEST(SocketOptions, SetNonblockingOnValidSocket) {
    net_init();
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE((int)s, (int)INVALID_SOCK);

    ASSERT_TRUE(net_set_nonblocking(s, true));
    ASSERT_TRUE(net_set_nonblocking(s, false));

    sock_close(s);
}

TEST(SocketOptions, SetNodelay_OnValidSocket) {
    net_init();
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE((int)s, (int)INVALID_SOCK);

    ASSERT_TRUE(net_set_nodelay(s));

    sock_close(s);
}

TEST(SocketOptions, SetReuseAddr_OnValidSocket) {
    net_init();
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE((int)s, (int)INVALID_SOCK);

    ASSERT_TRUE(net_set_reuseaddr(s));

    sock_close(s);
}

TEST(SocketOptions, SetNonblockingOnInvalidSocket) {
    net_init();
    ASSERT_FALSE(net_set_nonblocking(INVALID_SOCK, true));
}

TEST(SocketOptions, SetNodelayOnInvalidSocket) {
    net_init();
    ASSERT_FALSE(net_set_nodelay(INVALID_SOCK));
}

TEST(SocketOptions, SetReuseaddrOnInvalidSocket) {
    net_init();
    ASSERT_FALSE(net_set_reuseaddr(INVALID_SOCK));
}

TEST(NetErrorStr, ReturnsNonEmptyString) {
    net_init();
    // Force an error: try to bind a socket on port 0 with an invalid address
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s != INVALID_SOCK) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(0);
        addr.sin_addr.s_addr = inet_addr("300.300.300.300");  // invalid IP
        ::bind(s, (struct sockaddr*)&addr, sizeof(addr));     // will fail
        sock_close(s);
    }
    std::string err = net_error_str();
    ASSERT_FALSE(err.empty());
}

int main() { return RUN_ALL_TESTS(); }
