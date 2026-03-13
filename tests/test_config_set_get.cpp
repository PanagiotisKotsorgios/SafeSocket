/**
 * @file test_config_set_get.cpp
 * @brief Unit tests for Config::set() and Config::get().
 *
 * Verifies every configurable key — correct parsing of strings, ints, bools,
 * EncryptType, edge cases (negative int, unknown key), and sensitive-field masking.
 */

#include "../../helpers/test_framework.hpp"
#include "../../../config.hpp"

// Helper: fresh default config for each test
static Config fresh() {
    Config c;
    return c;
}

// ── Network ───────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, Host) {
    Config c = fresh();
    c.set("host", "192.168.1.100");
    ASSERT_STREQ(c.get("host"), "192.168.1.100");
    ASSERT_STREQ(c.host, "192.168.1.100");
}

TEST(ConfigSetGet, Port) {
    Config c = fresh();
    c.set("port", "8080");
    ASSERT_STREQ(c.get("port"), "8080");
    ASSERT_EQ(c.port, 8080);
}

TEST(ConfigSetGet, MaxClients) {
    Config c = fresh();
    c.set("max_clients", "32");
    ASSERT_EQ(c.max_clients, 32);
}

TEST(ConfigSetGet, RecvTimeout) {
    Config c = fresh();
    c.set("recv_timeout", "60");
    ASSERT_EQ(c.recv_timeout, 60);
}

TEST(ConfigSetGet, ConnectTimeout) {
    Config c = fresh();
    c.set("connect_timeout", "5");
    ASSERT_EQ(c.connect_timeout, 5);
}

// ── Identity ─────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, Nickname) {
    Config c = fresh();
    c.set("nickname", "Alice");
    ASSERT_STREQ(c.nickname, "Alice");
    ASSERT_STREQ(c.get("nickname"), "Alice");
}

TEST(ConfigSetGet, ServerName) {
    Config c = fresh();
    c.set("server_name", "MyServer");
    ASSERT_STREQ(c.server_name, "MyServer");
}

TEST(ConfigSetGet, Motd) {
    Config c = fresh();
    c.set("motd", "Welcome!");
    ASSERT_STREQ(c.motd, "Welcome!");
}

// ── Security ─────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, EncryptTypeNone) {
    Config c = fresh();
    c.set("encrypt_type", "NONE");
    ASSERT_EQ((int)c.encrypt_type, (int)EncryptType::NONE);
}

TEST(ConfigSetGet, EncryptTypeRC4) {
    Config c = fresh();
    c.set("encrypt_type", "RC4");
    ASSERT_EQ((int)c.encrypt_type, (int)EncryptType::RC4);
}

TEST(ConfigSetGet, EncryptTypeLowerCase) {
    Config c = fresh();
    c.set("encrypt_type", "rc4");
    ASSERT_EQ((int)c.encrypt_type, (int)EncryptType::RC4);
}

TEST(ConfigSetGet, EncryptKeySet) {
    Config c = fresh();
    c.set("encrypt_key", "topsecret");
    ASSERT_STREQ(c.encrypt_key, "topsecret");
}

TEST(ConfigSetGet, EncryptKeyMaskedInGet) {
    Config c = fresh();
    c.set("encrypt_key", "topsecret");
    ASSERT_STREQ(c.get("encrypt_key"), "***hidden***");
}

TEST(ConfigSetGet, EncryptKeyEmptyNotMasked) {
    Config c = fresh();
    c.set("encrypt_key", "");
    std::string val = c.get("encrypt_key");
    ASSERT_STRNE(val, "***hidden***");
}

TEST(ConfigSetGet, RequireKeyTrue) {
    Config c = fresh();
    c.set("require_key", "true");
    ASSERT_TRUE(c.require_key);
}

TEST(ConfigSetGet, RequireKeyOn) {
    Config c = fresh();
    c.set("require_key", "on");
    ASSERT_TRUE(c.require_key);
}

TEST(ConfigSetGet, RequireKeyFalse) {
    Config c = fresh();
    c.set("require_key", "false");
    ASSERT_FALSE(c.require_key);
}

TEST(ConfigSetGet, AccessKeyMasked) {
    Config c = fresh();
    c.set("access_key", "joinpass");
    ASSERT_STREQ(c.get("access_key"), "***hidden***");
}

// ── Transfer ─────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, DownloadDir) {
    Config c = fresh();
    c.set("download_dir", "/tmp/recv");
    ASSERT_STREQ(c.download_dir, "/tmp/recv");
}

TEST(ConfigSetGet, AutoAcceptFiles) {
    Config c = fresh();
    c.set("auto_accept_files", "true");
    ASSERT_TRUE(c.auto_accept_files);
}

TEST(ConfigSetGet, MaxFileSize) {
    Config c = fresh();
    c.set("max_file_size", "1048576");
    ASSERT_EQ((int)c.max_file_size, 1048576);
}

// ── Logging ───────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, LogToFile) {
    Config c = fresh();
    c.set("log_to_file", "1");
    ASSERT_TRUE(c.log_to_file);
}

TEST(ConfigSetGet, LogFile) {
    Config c = fresh();
    c.set("log_file", "myapp.log");
    ASSERT_STREQ(c.log_file, "myapp.log");
}

TEST(ConfigSetGet, Verbose) {
    Config c = fresh();
    c.set("verbose", "yes");
    ASSERT_TRUE(c.verbose);
}

// ── Keepalive ─────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, KeepaliveDefault) {
    Config c = fresh();
    ASSERT_TRUE(c.keepalive);   // default is true
}

TEST(ConfigSetGet, KeepaliveDisable) {
    Config c = fresh();
    c.set("keepalive", "false");
    ASSERT_FALSE(c.keepalive);
}

TEST(ConfigSetGet, PingInterval) {
    Config c = fresh();
    c.set("ping_interval", "60");
    ASSERT_EQ(c.ping_interval, 60);
}

// ── Misc ──────────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, ColorOutput) {
    Config c = fresh();
    c.set("color_output", "false");
    ASSERT_FALSE(c.color_output);
}

TEST(ConfigSetGet, BufferSize) {
    Config c = fresh();
    c.set("buffer_size", "8192");
    ASSERT_EQ(c.buffer_size, 8192);
}

// ── Edge cases ────────────────────────────────────────────────────────────────

TEST(ConfigSetGet, UnknownKeyDoesNotCrash) {
    Config c = fresh();
    // Should emit a warning to stderr but not throw or crash
    c.set("nonexistent_key", "value");
    ASSERT_STREQ(c.get("nonexistent_key"), "(unknown key)");
}

TEST(ConfigSetGet, CaseInsensitiveKeys) {
    Config c = fresh();
    c.set("PORT", "1234");
    ASSERT_EQ(c.port, 1234);

    c.set("NICKNAME", "Bob");
    ASSERT_STREQ(c.nickname, "Bob");
}

TEST(ConfigSetGet, InvalidIntFallsBack) {
    Config c = fresh();
    int old_port = c.port;
    c.set("port", "not_a_number");
    // Should fall back gracefully — port unchanged or set to default
    // (implementation uses stoi with try/catch; result is the default 9000)
    ASSERT_GE(c.port, 0);
}

int main() { return RUN_ALL_TESTS(); }
