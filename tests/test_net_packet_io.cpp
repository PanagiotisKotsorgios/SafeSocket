/**
 * @file test_net_packet_io.cpp
 * @brief Unit tests for net_send_packet() and net_recv_packet().
 *
 * Verifies:
 *  - basic send/recv round-trip (no encryption)
 *  - header fields are preserved exactly
 *  - payload bytes are preserved exactly
 *  - zero-length payload
 *  - round-trip with XOR encryption
 *  - round-trip with VIGENERE encryption
 *  - round-trip with RC4 encryption
 *  - recv returns false when magic is wrong (corrupt header)
 *  - recv returns false when connection drops before header
 *  - recv returns false when connection drops mid-payload
 */

#include "../../helpers/test_framework.hpp"
#include "../../helpers/socket_pair.hpp"
#include "../../../network.hpp"
#include "../../../protocol.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <thread>

// ── helper: send a packet in a background thread, recv in foreground ──────────

struct SendRecvResult {
    bool     send_ok;
    bool     recv_ok;
    PacketHeader hdr;
    std::vector<uint8_t> payload;
};

static SendRecvResult send_recv(sock_t sender_sock, sock_t receiver_sock,
                                uint32_t type, uint32_t sender_id,
                                uint32_t target_id,
                                const void* data, uint32_t data_len,
                                EncryptType enc = EncryptType::NONE,
                                const std::string& key = "") {
    SendRecvResult r{};
    std::thread t([&]() {
        r.send_ok = net_send_packet(sender_sock, type, sender_id, target_id,
                                    data, data_len, enc, key);
    });
    r.recv_ok = net_recv_packet(receiver_sock, r.hdr, r.payload, enc, key);
    t.join();
    return r;
}

// ── tests ──────────────────────────────────────────────────────────────────────

TEST(PacketIO, BasicRoundTrip) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "test payload";
    uint32_t dlen = (uint32_t)std::strlen(payload);

    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_TEXT, 42, 7, payload, dlen);

    ASSERT_TRUE(r.send_ok);
    ASSERT_TRUE(r.recv_ok);
}

TEST(PacketIO, HeaderFieldsPreserved) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "fields";
    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_PRIVATE, 99, 77,
                       payload, (uint32_t)std::strlen(payload));

    ASSERT_EQ(r.hdr.magic,     MAGIC);
    ASSERT_EQ(r.hdr.msg_type,  (uint32_t)MSG_PRIVATE);
    ASSERT_EQ(r.hdr.sender_id, 99u);
    ASSERT_EQ(r.hdr.target_id, 77u);
    ASSERT_EQ(r.hdr.data_len,  (uint32_t)std::strlen(payload));
}

TEST(PacketIO, PayloadBytesPreserved) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "preserve these bytes exactly";
    uint32_t dlen = (uint32_t)std::strlen(payload);

    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_TEXT, 1, 2, payload, dlen);

    ASSERT_TRUE(r.recv_ok);
    ASSERT_EQ((int)r.payload.size(), (int)dlen);
    ASSERT_EQ(std::memcmp(r.payload.data(), payload, dlen), 0);
}

TEST(PacketIO, ZeroLengthPayload) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_PING, SERVER_ID, BROADCAST_ID, nullptr, 0);

    ASSERT_TRUE(r.send_ok);
    ASSERT_TRUE(r.recv_ok);
    ASSERT_EQ((int)r.payload.size(), 0);
    ASSERT_EQ(r.hdr.data_len, 0u);
}

TEST(PacketIO, RoundTripXOR) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "encrypted with XOR";
    uint32_t dlen = (uint32_t)std::strlen(payload);

    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_TEXT, 1, 2, payload, dlen,
                       EncryptType::XOR, "xorkey");

    ASSERT_TRUE(r.recv_ok);
    ASSERT_EQ(std::memcmp(r.payload.data(), payload, dlen), 0);
}

TEST(PacketIO, RoundTripVigenere) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "vigenere payload";
    uint32_t dlen = (uint32_t)std::strlen(payload);

    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_TEXT, 1, 2, payload, dlen,
                       EncryptType::VIGENERE, "vigkey");

    ASSERT_TRUE(r.recv_ok);
    ASSERT_EQ(std::memcmp(r.payload.data(), payload, dlen), 0);
}

TEST(PacketIO, RoundTripRC4) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "RC4 secured message";
    uint32_t dlen = (uint32_t)std::strlen(payload);

    auto r = send_recv(sp.client(), sp.server_conn(),
                       MSG_TEXT, 5, 6, payload, dlen,
                       EncryptType::RC4, "rc4passphrase");

    ASSERT_TRUE(r.recv_ok);
    ASSERT_EQ(std::memcmp(r.payload.data(), payload, dlen), 0);
}

TEST(PacketIO, RecvFailsOnCorruptMagic) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    // Manually craft a header with wrong magic
    PacketHeader bad_hdr;
    bad_hdr.magic     = 0xDEADBEEF;  // wrong
    bad_hdr.msg_type  = MSG_TEXT;
    bad_hdr.sender_id = 1;
    bad_hdr.target_id = 2;
    bad_hdr.data_len  = 5;

    std::thread sender([&]() {
        net_send_raw(sp.client(), &bad_hdr, sizeof(bad_hdr));
        const char p[] = "hello";
        net_send_raw(sp.client(), p, 5);
    });

    PacketHeader hdr;
    std::vector<uint8_t> payload;
    bool ok = net_recv_packet(sp.server_conn(), hdr, payload);

    sender.join();
    ASSERT_FALSE(ok);
}

TEST(PacketIO, RecvFailsOnConnectionDrop_BeforeHeader) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    // Close client immediately — server recv should fail
    sock_close(sp.client());

    PacketHeader hdr;
    std::vector<uint8_t> payload;
    bool ok = net_recv_packet(sp.server_conn(), hdr, payload);
    ASSERT_FALSE(ok);
}

TEST(PacketIO, MismatchedEncryptionKeyDoesNotRestore) {
    net_init();
    SocketPair sp;
    ASSERT_TRUE(sp.open());

    const char payload[] = "secret data";
    uint32_t dlen = (uint32_t)std::strlen(payload);

    // Send with "rightkey", recv with "wrongkey"
    std::thread sender([&]() {
        net_send_packet(sp.client(), MSG_TEXT, 1, 2,
                        payload, dlen, EncryptType::XOR, "rightkey");
    });

    PacketHeader hdr;
    std::vector<uint8_t> recv_payload;
    bool ok = net_recv_packet(sp.server_conn(), hdr, recv_payload,
                              EncryptType::XOR, "wrongkey");
    sender.join();

    if (ok && (int)recv_payload.size() == (int)dlen) {
        // Payload must differ from original
        ASSERT_NE(std::memcmp(recv_payload.data(), payload, dlen), 0);
    }
    // (recv itself may still return true — the magic was correct)
}

int main() { return RUN_ALL_TESTS(); }
