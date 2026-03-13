/**
 * @file test_packet_header.cpp
 * @brief Unit tests for PacketHeader, make_packet(), and make_text_packet().
 *
 * Verifies:
 *  - PacketHeader is exactly 20 bytes (no padding)
 *  - MAGIC constant value
 *  - HEADER_SIZE == sizeof(PacketHeader)
 *  - make_packet() produces correct total size
 *  - make_packet() sets all header fields correctly
 *  - make_packet() copies payload bytes correctly
 *  - make_packet() with zero-length payload
 *  - make_text_packet() matches make_packet() for string payloads
 *  - SERVER_ID and BROADCAST_ID values
 *  - FileStartPayload size and field layout
 */

#include "../../helpers/test_framework.hpp"
#include "../../../protocol.hpp"
#include <cstring>

TEST(PacketHeader, SizeIs20Bytes) {
    ASSERT_EQ((int)sizeof(PacketHeader), 20);
}

TEST(PacketHeader, MagicConstantValue) {
    ASSERT_EQ(MAGIC, 0x534B5A4Fu);
}

TEST(PacketHeader, HeaderSizeMatchesSizeof) {
    ASSERT_EQ(HEADER_SIZE, sizeof(PacketHeader));
}

TEST(PacketHeader, FieldOffsets) {
    PacketHeader h;
    // Verify offsets match wire spec using offsetof
    ASSERT_EQ(offsetof(PacketHeader, magic),     (size_t)0);
    ASSERT_EQ(offsetof(PacketHeader, msg_type),  (size_t)4);
    ASSERT_EQ(offsetof(PacketHeader, sender_id), (size_t)8);
    ASSERT_EQ(offsetof(PacketHeader, target_id), (size_t)12);
    ASSERT_EQ(offsetof(PacketHeader, data_len),  (size_t)16);
}

TEST(PacketHeader, ServerIdIsZero) {
    ASSERT_EQ(SERVER_ID, 0u);
}

TEST(PacketHeader, BroadcastIdIsMaxUint) {
    ASSERT_EQ(BROADCAST_ID, 0xFFFFFFFFu);
}

TEST(PacketHeader, MakePacketTotalSize) {
    const char payload[] = "hello";
    auto pkt = make_packet(MSG_TEXT, 1, BROADCAST_ID,
                           payload, (uint32_t)std::strlen(payload));
    ASSERT_EQ(pkt.size(), HEADER_SIZE + std::strlen(payload));
}

TEST(PacketHeader, MakePacketHeaderFields) {
    const char payload[] = "data";
    uint32_t dlen = (uint32_t)std::strlen(payload);
    auto pkt = make_packet(MSG_PRIVATE, 42, 7, payload, dlen);

    const PacketHeader* h = reinterpret_cast<const PacketHeader*>(pkt.data());
    ASSERT_EQ(h->magic,     MAGIC);
    ASSERT_EQ(h->msg_type,  (uint32_t)MSG_PRIVATE);
    ASSERT_EQ(h->sender_id, 42u);
    ASSERT_EQ(h->target_id, 7u);
    ASSERT_EQ(h->data_len,  dlen);
}

TEST(PacketHeader, MakePacketPayloadCopied) {
    const char payload[] = "payload bytes";
    uint32_t dlen = (uint32_t)std::strlen(payload);
    auto pkt = make_packet(MSG_TEXT, 1, 2, payload, dlen);

    ASSERT_EQ(pkt.size(), HEADER_SIZE + dlen);
    ASSERT_EQ(std::memcmp(pkt.data() + HEADER_SIZE, payload, dlen), 0);
}

TEST(PacketHeader, MakePacketZeroLengthPayload) {
    auto pkt = make_packet(MSG_PING, SERVER_ID, BROADCAST_ID, nullptr, 0);
    ASSERT_EQ(pkt.size(), HEADER_SIZE);

    const PacketHeader* h = reinterpret_cast<const PacketHeader*>(pkt.data());
    ASSERT_EQ(h->magic,    MAGIC);
    ASSERT_EQ(h->msg_type, (uint32_t)MSG_PING);
    ASSERT_EQ(h->data_len, 0u);
}

TEST(PacketHeader, MakeTextPacketMatchesMakePacket) {
    std::string text = "Hello World";
    auto pkt1 = make_text_packet(MSG_BROADCAST, 1, BROADCAST_ID, text);
    auto pkt2 = make_packet(MSG_BROADCAST, 1, BROADCAST_ID,
                            text.c_str(), (uint32_t)text.size());
    ASSERT_TRUE(pkt1 == pkt2);
}

TEST(PacketHeader, MakeTextPacketPayloadNotNullTerminated) {
    std::string text = "no null";
    auto pkt = make_text_packet(MSG_TEXT, 0, 0, text);

    const PacketHeader* h = reinterpret_cast<const PacketHeader*>(pkt.data());
    ASSERT_EQ(h->data_len, (uint32_t)text.size());
    // The byte immediately after the payload is not part of the packet
    ASSERT_EQ(pkt.size(), HEADER_SIZE + text.size());
}

TEST(PacketHeader, AllMsgTypesHaveDistinctValues) {
    uint32_t types[] = {
        MSG_CONNECT, MSG_DISCONNECT, MSG_TEXT, MSG_BROADCAST,
        MSG_PRIVATE, MSG_CLIENT_LIST, MSG_FILE_START, MSG_FILE_DATA,
        MSG_FILE_END, MSG_ACK, MSG_ERROR, MSG_NICK_SET, MSG_SERVER_INFO,
        MSG_PING, MSG_PONG, MSG_KICK, MSG_FILE_ACCEPT, MSG_FILE_REJECT
    };
    const int N = sizeof(types) / sizeof(types[0]);
    for (int i = 0; i < N; ++i)
        for (int j = i + 1; j < N; ++j)
            ASSERT_NE(types[i], types[j]);
}

TEST(PacketHeader, FileStartPayloadSize) {
    // file_size(8) + filename_len(4) = 12 bytes minimum
    ASSERT_EQ(sizeof(FileStartPayload), (size_t)12);
    ASSERT_EQ(offsetof(FileStartPayload, file_size),    (size_t)0);
    ASSERT_EQ(offsetof(FileStartPayload, filename_len), (size_t)8);
}

TEST(PacketHeader, FileChunkConstant) {
    ASSERT_EQ(FILE_CHUNK, (size_t)65536);
}

TEST(PacketHeader, MaxClientsConstant) {
    ASSERT_EQ(MAX_CLIENTS, 64);
}

int main() { return RUN_ALL_TESTS(); }
