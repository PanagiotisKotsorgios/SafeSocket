/**
 * @file protocol.hpp
 * @brief Shared packet structures, message types, and protocol constants for SafeSocket.
 *
 * This header defines the complete wire protocol used by SafeSocket for all
 * client-server communication. It includes message type enumerations, binary
 * packet structures, special IDs, protocol constants, and inline utility
 * functions for building packets.
 *
 * All multi-byte fields are stored in host byte order unless explicitly noted.
 * Consumers are responsible for byte-order conversions where interoperability
 * across different architectures is required.
 *
 * @author  SafeSocket Project
 * @version 1.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

// ============================================================
//  Message Types
// ============================================================

/**
 * @brief Enumeration of all message types used in the SafeSocket protocol.
 *
 * Each value identifies the purpose of a packet and determines how the
 * receiver should interpret the payload. The underlying type is @c uint32_t
 * to match the @c msg_type field of @ref PacketHeader.
 */
enum MsgType : uint32_t {
    MSG_CONNECT      = 1,   ///< Client hello / registration. Payload: [uint16_t nick_len][nick bytes][optional access key].
    MSG_DISCONNECT   = 2,   ///< Graceful disconnect notification. No payload required.
    MSG_TEXT         = 3,   ///< Plain chat text message.
    MSG_BROADCAST    = 4,   ///< Broadcast text to every connected client.
    MSG_PRIVATE      = 5,   ///< Private text directed to a specific client.
    MSG_CLIENT_LIST  = 6,   ///< Server?client: serialized list of currently online clients.
    MSG_FILE_START   = 7,   ///< Begin a file transfer; payload is @ref FileStartPayload followed by the filename.
    MSG_FILE_DATA    = 8,   ///< Raw file chunk during an active file transfer.
    MSG_FILE_END     = 9,   ///< Signals the end of a file transfer; no payload.
    MSG_ACK          = 10,  ///< Generic acknowledgment; optional text payload for context.
    MSG_ERROR        = 11,  ///< Error message; payload is a human-readable error string.
    MSG_NICK_SET     = 12,  ///< Request to set or change the sender's nickname; payload is the new nickname.
    MSG_SERVER_INFO  = 13,  ///< Server information / MOTD sent to a newly connected client.
    MSG_PING         = 14,  ///< Keepalive ping; no payload expected.
    MSG_PONG         = 15,  ///< Keepalive pong in response to @ref MSG_PING; no payload.
    MSG_KICK         = 16,  ///< Server kicks a client; payload is an optional reason string.
    MSG_FILE_ACCEPT  = 17,  ///< Receiver accepts an incoming file transfer; no payload.
    MSG_FILE_REJECT  = 18,  ///< Receiver rejects an incoming file transfer; no payload.
};

// ============================================================
//  Special IDs
// ============================================================

/**
 * @brief Numeric ID reserved for the server itself.
 *
 * Used in the @c sender_id or @c target_id fields of @ref PacketHeader
 * to indicate that the server is the originator or the sole recipient.
 */
#define SERVER_ID      0u

/**
 * @brief Sentinel target ID indicating a broadcast to all connected clients.
 *
 * When @c target_id is set to this value the server should relay the packet
 * to every client currently in the session.
 */
#define BROADCAST_ID   0xFFFFFFFFu

// ============================================================
//  Packet Header
// ============================================================

/**
 * @brief Fixed-size packet header transmitted before every payload.
 *
 * The header is exactly 20 bytes and is packed to avoid compiler-inserted
 * padding. It must be sent and received as a complete atomic unit before the
 * accompanying payload bytes.
 *
 * Wire layout (20 bytes total):
 * @code
 * Offset  Size  Field
 *      0     4  magic
 *      4     4  msg_type
 *      8     4  sender_id
 *     12     4  target_id
 *     16     4  data_len
 * @endcode
 */
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic;      ///< Protocol magic number; must equal @ref MAGIC (0x534B5A4F).
    uint32_t msg_type;   ///< One of the @ref MsgType values identifying the packet's purpose.
    uint32_t sender_id;  ///< Numeric ID of the originating client, or @ref SERVER_ID for server-originated packets.
    uint32_t target_id;  ///< Numeric ID of the intended recipient, or @ref BROADCAST_ID for broadcasts.
    uint32_t data_len;   ///< Length in bytes of the payload that immediately follows this header.
};
#pragma pack(pop)

// ============================================================
//  Protocol Constants
// ============================================================

/**
 * @brief Magic number placed at the start of every @ref PacketHeader.
 *
 * Receivers verify this value to detect misaligned reads, corrupted streams,
 * or connections from unrelated software. Value spells "OKZS" in ASCII.
 */
static const uint32_t MAGIC = 0x534B5A4Fu;

/**
 * @brief Size in bytes of a @ref PacketHeader.
 *
 * Equivalent to @c sizeof(PacketHeader); provided as a named constant for
 * clarity at every call site that deals with raw buffer arithmetic.
 */
static const size_t   HEADER_SIZE = sizeof(PacketHeader);

/**
 * @brief Maximum size of a single file-data chunk in bytes (64 KiB).
 *
 * File payloads are split into chunks no larger than this value and sent as
 * individual @ref MSG_FILE_DATA packets during a file transfer session.
 */
static const size_t   FILE_CHUNK  = 65536;

/**
 * @brief Hard upper limit on the number of simultaneous client connections.
 *
 * The server will reject new connections once this many clients are active.
 * This default may be overridden via @ref Config::max_clients.
 */
static const int      MAX_CLIENTS = 64;

// ============================================================
//  MSG_CONNECT Payload Layout (documentation only)
// ============================================================
// Payload: [uint16_t nick_len][nick bytes][optional access key bytes]
// There is no null terminator stored for either string field.

// ============================================================
//  MSG_FILE_START Payload Structure
// ============================================================

/**
 * @brief Fixed-length prefix of a @ref MSG_FILE_START payload.
 *
 * Immediately follows @ref PacketHeader in a @ref MSG_FILE_START packet.
 * The filename string (without null terminator) is appended directly after
 * this structure; its length is given by @c filename_len.
 *
 * Wire layout:
 * @code
 * Offset  Size  Field
 *      0     8  file_size
 *      8     4  filename_len
 *     12  fname  <filename bytes>
 * @endcode
 */
#pragma pack(push, 1)
struct FileStartPayload {
    uint64_t file_size;    ///< Total size of the file being transferred, in bytes.
    uint32_t filename_len; ///< Number of bytes in the filename that follows this struct.
    // Filename bytes follow immediately (no null terminator stored).
};
#pragma pack(pop)

// ============================================================
//  MSG_CLIENT_LIST Payload Layout (documentation only)
// ============================================================
// [uint32_t count]
// For each entry: [uint32_t id][uint16_t nick_len][nick bytes]

// ============================================================
//  Inline Utility Functions
// ============================================================

/**
 * @brief Builds a complete raw packet (header + payload) in a byte vector.
 *
 * The returned vector contains a fully formed @ref PacketHeader followed
 * immediately by a copy of @p data. No encryption is applied here; callers
 * that need encryption should use @c net_send_packet() instead.
 *
 * @param type      @ref MsgType value identifying the packet purpose.
 * @param sender    Numeric ID of the originating client or @ref SERVER_ID.
 * @param target    Numeric ID of the intended recipient or @ref BROADCAST_ID.
 * @param data      Pointer to the payload bytes; may be @c nullptr when @p data_len is 0.
 * @param data_len  Number of payload bytes pointed to by @p data.
 * @return          Byte vector containing the complete packet ready for transmission.
 */
inline std::vector<uint8_t> make_packet(uint32_t type,
                                        uint32_t sender,
                                        uint32_t target,
                                        const void* data,
                                        uint32_t data_len)
{
    std::vector<uint8_t> buf(HEADER_SIZE + data_len);
    PacketHeader* h = reinterpret_cast<PacketHeader*>(buf.data());
    h->magic     = MAGIC;
    h->msg_type  = type;
    h->sender_id = sender;
    h->target_id = target;
    h->data_len  = data_len;
    if (data && data_len > 0)
        memcpy(buf.data() + HEADER_SIZE, data, data_len);
    return buf;
}

/**
 * @brief Convenience wrapper around @ref make_packet() for text payloads.
 *
 * Converts @p text to a raw byte range and delegates to @ref make_packet().
 * The string is not null-terminated in the packet; the receiver reconstructs
 * it from @c data_len in the header.
 *
 * @param type    @ref MsgType value identifying the packet purpose.
 * @param sender  Numeric ID of the originating client or @ref SERVER_ID.
 * @param target  Numeric ID of the intended recipient or @ref BROADCAST_ID.
 * @param text    UTF-8 string to use as the packet payload.
 * @return        Byte vector containing the complete packet ready for transmission.
 */
inline std::vector<uint8_t> make_text_packet(uint32_t type,
                                              uint32_t sender,
                                              uint32_t target,
                                              const std::string& text)
{
    return make_packet(type, sender, target,
                       text.c_str(), static_cast<uint32_t>(text.size()));
}
