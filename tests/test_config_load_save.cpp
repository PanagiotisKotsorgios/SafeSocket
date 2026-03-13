/**
 * @file test_config_load_save.cpp
 * @brief Unit tests for Config::load() and Config::save().
 *
 * Verifies:
 *  - save() writes a readable file
 *  - load() round-trips all fields correctly
 *  - load() handles comments (# and ;)
 *  - load() handles inline comments
 *  - load() silently skips unknown keys
 *  - load() returns false for nonexistent file
 *  - empty lines are skipped
 *  - values with spaces are preserved
 *  - save() returns false on bad path
 */

#include "../../helpers/test_framework.hpp"
#include "../../helpers/temp_file.hpp"
#include "../../../config.hpp"
#include <fstream>
#include <string>

TEST(ConfigLoadSave, SaveCreatesFile) {
    TempFile tf;
    Config c;
    ASSERT_TRUE(c.save(tf.path()));
    ASSERT_TRUE(tf.exists());
}

TEST(ConfigLoadSave, RoundTripAllFields) {
    TempFile tf;
    Config src;
    src.host             = "10.0.0.1";
    src.port             = 8888;
    src.max_clients      = 16;
    src.recv_timeout     = 120;
    src.connect_timeout  = 7;
    src.nickname         = "RoundTrip";
    src.server_name      = "TestServer";
    src.motd             = "Hello from test";
    src.encrypt_type     = EncryptType::RC4;
    src.encrypt_key      = "testsecret";
    src.require_key      = true;
    src.access_key       = "joinkey";
    src.download_dir     = "/tmp/dl";
    src.auto_accept_files= true;
    src.max_file_size    = 2048;
    src.log_to_file      = true;
    src.log_file         = "test.log";
    src.verbose          = true;
    src.keepalive        = false;
    src.ping_interval    = 45;
    src.color_output     = false;
    src.buffer_size      = 512;

    ASSERT_TRUE(src.save(tf.path()));

    Config dst;
    ASSERT_TRUE(dst.load(tf.path()));

    ASSERT_STREQ(dst.host,         "10.0.0.1");
    ASSERT_EQ(dst.port,            8888);
    ASSERT_EQ(dst.max_clients,     16);
    ASSERT_EQ(dst.recv_timeout,    120);
    ASSERT_EQ(dst.connect_timeout, 7);
    ASSERT_STREQ(dst.nickname,     "RoundTrip");
    ASSERT_STREQ(dst.server_name,  "TestServer");
    ASSERT_STREQ(dst.motd,         "Hello from test");
    ASSERT_EQ((int)dst.encrypt_type, (int)EncryptType::RC4);
    ASSERT_STREQ(dst.encrypt_key,  "testsecret");
    ASSERT_TRUE(dst.require_key);
    ASSERT_STREQ(dst.access_key,   "joinkey");
    ASSERT_STREQ(dst.download_dir, "/tmp/dl");
    ASSERT_TRUE(dst.auto_accept_files);
    ASSERT_EQ((int)dst.max_file_size, 2048);
    ASSERT_TRUE(dst.log_to_file);
    ASSERT_STREQ(dst.log_file,     "test.log");
    ASSERT_TRUE(dst.verbose);
    ASSERT_FALSE(dst.keepalive);
    ASSERT_EQ(dst.ping_interval,   45);
    ASSERT_FALSE(dst.color_output);
    ASSERT_EQ(dst.buffer_size,     512);
}

TEST(ConfigLoadSave, LoadReturnsFalseForMissingFile) {
    Config c;
    ASSERT_FALSE(c.load("/tmp/this_file_does_not_exist_safesocket_test.conf"));
}

TEST(ConfigLoadSave, SkipsHashComments) {
    TempFile tf("# this is a comment\nport = 7777\n");
    Config c;
    ASSERT_TRUE(c.load(tf.path()));
    ASSERT_EQ(c.port, 7777);
}

TEST(ConfigLoadSave, SkipsSemicolonComments) {
    TempFile tf("; semicolon comment\nport = 6666\n");
    Config c;
    ASSERT_TRUE(c.load(tf.path()));
    ASSERT_EQ(c.port, 6666);
}

TEST(ConfigLoadSave, SkipsEmptyLines) {
    TempFile tf("\n\n\nport = 5555\n\n");
    Config c;
    ASSERT_TRUE(c.load(tf.path()));
    ASSERT_EQ(c.port, 5555);
}

TEST(ConfigLoadSave, InlineCommentStripped) {
    TempFile tf("port = 4444 # inline comment\n");
    Config c;
    ASSERT_TRUE(c.load(tf.path()));
    ASSERT_EQ(c.port, 4444);
}

TEST(ConfigLoadSave, UnknownKeySilentlySkipped) {
    TempFile tf("unknown_key_xyz = 999\nport = 3333\n");
    Config c;
    ASSERT_TRUE(c.load(tf.path()));  // should not fail
    ASSERT_EQ(c.port, 3333);
}

TEST(ConfigLoadSave, WhitespaceAroundEquals) {
    TempFile tf("  port  =  2222  \n");
    Config c;
    ASSERT_TRUE(c.load(tf.path()));
    ASSERT_EQ(c.port, 2222);
}

TEST(ConfigLoadSave, SaveReturnsFalseOnBadPath) {
    Config c;
    ASSERT_FALSE(c.save("/no/such/directory/config.conf"));
}

TEST(ConfigLoadSave, DefaultsUnchangedForMissingKeys) {
    TempFile tf("port = 1111\n");  // only port specified
    Config c;
    ASSERT_TRUE(c.load(tf.path()));
    ASSERT_EQ(c.port, 1111);
    // Everything else should still be default
    ASSERT_STREQ(c.host, "127.0.0.1");
    ASSERT_STREQ(c.nickname, "anonymous");
    ASSERT_EQ((int)c.encrypt_type, (int)EncryptType::NONE);
}

int main() { return RUN_ALL_TESTS(); }
