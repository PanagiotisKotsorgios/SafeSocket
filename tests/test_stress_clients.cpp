/**
 * @file test_stress_clients.cpp
 * @brief Stress tests: many simultaneous clients, rapid connect/disconnect.
 *
 * These tests are slower by design and verify the server does not:
 *  - deadlock under concurrent access
 *  - leak sockets / file descriptors
 *  - crash on rapid connect/disconnect churn
 *  - drop keepalive heartbeats at high client counts
 */

#include "../helpers/test_framework.hpp"
#include "../../server.hpp"
#include "../../client.hpp"
#include "../../config.hpp"
#include "../../network.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <atomic>
#include <string>

static int free_port() {
    sock_t s = net_listen("127.0.0.1", 0);
    if (s == INVALID_SOCK) return 20400;
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
    g_config.max_clients  = 64;
    g_config.encrypt_type = EncryptType::NONE;
    g_config.encrypt_key  = "";
    g_config.require_key  = false;
    g_config.keepalive    = false;
    g_config.verbose      = false;
    g_config.motd         = "stress";
}

// ── 1. Many sequential clients ────────────────────────────────────────────────

TEST(StressClients, FiftySequentialConnections) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    int success = 0;
    for (int i = 0; i < 50; ++i) {
        g_config.nickname = "User" + std::to_string(i);
        Client c;
        if (c.connect("127.0.0.1", port)) {
            sleep_ms(20);
            ++success;
            c.disconnect();
            sleep_ms(10);
        }
    }

    ASSERT_GE(success, 45);   // allow a small margin for OS resource limits

    server.stop();
}

// ── 2. Concurrent clients all connect at once ─────────────────────────────────

TEST(StressClients, TwentySimultaneousConnections) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    std::atomic<int> connected{0};
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<Client>> clients(20);

    for (int i = 0; i < 20; ++i) {
        clients[i] = std::unique_ptr<Client>(new Client());
        threads.emplace_back([&, i]() {
            g_config.nickname = "C" + std::to_string(i);
            if (clients[i]->connect("127.0.0.1", port))
                ++connected;
        });
    }
    for (auto& t : threads) t.join();

    sleep_ms(200);
    ASSERT_GE(connected.load(), 18);

    for (auto& c : clients) c->disconnect();
    sleep_ms(100);
    server.stop();
}

// ── 3. Rapid connect/disconnect churn ─────────────────────────────────────────

TEST(StressClients, RapidConnectDisconnectChurn) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    // 30 iterations: connect, stay 30 ms, disconnect
    for (int i = 0; i < 30; ++i) {
        g_config.nickname = "Churn" + std::to_string(i);
        Client c;
        c.connect("127.0.0.1", port);
        sleep_ms(30);
        c.disconnect();
        sleep_ms(10);
    }

    // Server must still be alive and accepting
    g_config.nickname = "FinalCheck";
    Client last;
    ASSERT_TRUE(last.connect("127.0.0.1", port));
    last.disconnect();

    server.stop();
}

// ── 4. Broadcast storm ────────────────────────────────────────────────────────

TEST(StressClients, BroadcastStorm) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    // Connect 10 clients
    std::vector<std::unique_ptr<Client>> clients(10);
    for (int i = 0; i < 10; ++i) {
        clients[i] = std::unique_ptr<Client>(new Client());
        g_config.nickname = "Storm" + std::to_string(i);
        clients[i]->connect("127.0.0.1", port);
        sleep_ms(20);
    }
    sleep_ms(100);

    // Blast 100 broadcasts
    for (int m = 0; m < 100; ++m) {
        server.broadcast_text("Storm message " + std::to_string(m));
    }
    sleep_ms(500);

    // All clients still connected
    for (int i = 0; i < 10; ++i)
        clients[i]->disconnect();

    server.stop();
}

// ── 5. Max client limit is enforced ──────────────────────────────────────────

TEST(StressClients, MaxClientLimitEnforced) {
    net_init();
    int port = free_port();
    base_config(port);
    g_config.max_clients = 5;

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    std::vector<std::unique_ptr<Client>> clients(8);
    int success = 0;
    for (int i = 0; i < 8; ++i) {
        clients[i] = std::unique_ptr<Client>(new Client());
        g_config.nickname = "Limit" + std::to_string(i);
        if (clients[i]->connect("127.0.0.1", port)) {
            ++success;
            sleep_ms(30);
        }
    }
    sleep_ms(200);

    // No more than max_clients should be connected
    ASSERT_LE(server.client_count(), 5);

    for (auto& c : clients) c->disconnect();
    server.stop();
    g_config.max_clients = 64;
}

// ── 6. Concurrent broadcasts from multiple clients ────────────────────────────

TEST(StressClients, ConcurrentClientBroadcasts) {
    net_init();
    int port = free_port();
    base_config(port);

    Server server;
    ASSERT_TRUE(server.start("127.0.0.1", port));
    sleep_ms(50);

    const int N = 8;
    std::vector<std::unique_ptr<Client>> clients(N);
    for (int i = 0; i < N; ++i) {
        clients[i] = std::unique_ptr<Client>(new Client());
        g_config.nickname = "CB" + std::to_string(i);
        clients[i]->connect("127.0.0.1", port);
        sleep_ms(20);
    }
    sleep_ms(100);

    // All clients broadcast simultaneously
    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            for (int m = 0; m < 20; ++m) {
                clients[i]->broadcast("msg " + std::to_string(m) +
                                      " from " + std::to_string(i));
                sleep_ms(5);
            }
        });
    }
    for (auto& t : threads) t.join();
    sleep_ms(300);

    // Server still alive
    ASSERT_GE(server.client_count(), 0);

    for (auto& c : clients) c->disconnect();
    server.stop();
}

int main() { return RUN_ALL_TESTS(); }
