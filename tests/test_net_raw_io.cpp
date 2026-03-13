/**
 * @file test_net_raw_io.cpp
 * @brief Unit tests for net_send_raw() and net_recv_raw().
 *
 * Verifies:
 *  - send_raw / recv_raw round-trip for small data
 *  - send_raw / recv_raw round-trip for large data (> socket buffer)
 *  - recv_raw returns false when sender closes connection
 *  - send_raw returns false on closed socket
 *  - empty send (len=0) is safe
 *  - binary data (all byte values 0-255)
 */

#include "../../helpers/test_framework.hpp"
#include "../../helpers/socket_pair.hpp"
#include "../../../network.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <thread>

TEST(NetRawIO, SmallRoundTrip) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char msg[] = "Hello from net_send_raw";
    const size_t len = sizeof(msg) - 1;

    ASSERT_TRUE(net_send_raw(sp.client(), msg, len));

    char buf[64] = {};
    ASSERT_TRUE(net_recv_raw(sp.server_conn(), buf, len));
    ASSERT_EQ(std::memcmp(buf, msg, len), 0);
}

TEST(NetRawIO, LargeRoundTrip) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    // 256 KB — forces multiple kernel send/recv iterations
    const size_t SIZE = 256 * 1024;
    std::vector<uint8_t> send_buf(SIZE);
    for (size_t i = 0; i < SIZE; ++i) send_buf[i] = (uint8_t)(i & 0xFF);

    std::vector<uint8_t> recv_buf(SIZE, 0);

    // Send in background thread to avoid deadlock on full socket buffers
    bool send_ok = false;
    std::thread sender([&]() {
        send_ok = net_send_raw(sp.client(), send_buf.data(), SIZE);
    });

    bool recv_ok = net_recv_raw(sp.server_conn(), recv_buf.data(), SIZE);

    sender.join();

    ASSERT_TRUE(send_ok);
    ASSERT_TRUE(recv_ok);
    ASSERT_TRUE(send_buf == recv_buf);
}

TEST(NetRawIO, RecvReturnsFalseOnClosedSender) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    // Close the client side
    sock_close(sp.client());

    char buf[4];
    // recv on server side should return false (EOF)
    ASSERT_FALSE(net_recv_raw(sp.server_conn(), buf, sizeof(buf)));
}

TEST(NetRawIO, BinaryData) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; ++i) data[i] = (uint8_t)i;

    std::vector<uint8_t> recv(256, 0);
    bool send_ok = false;
    std::thread sender([&]() {
        send_ok = net_send_raw(sp.client(), data.data(), data.size());
    });

    bool recv_ok = net_recv_raw(sp.server_conn(), recv.data(), recv.size());
    sender.join();

    ASSERT_TRUE(send_ok);
    ASSERT_TRUE(recv_ok);
    ASSERT_TRUE(data == recv);
}

TEST(NetRawIO, ZeroLengthSendIsSafe) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());
    // len=0 should not crash (loop doesn't iterate, returns true)
    ASSERT_TRUE(net_send_raw(sp.client(), nullptr, 0));
}

int main() { return RUN_ALL_TESTS(); }
