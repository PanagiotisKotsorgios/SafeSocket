/**
 * @file socket_pair.hpp
 * @brief Helper that creates a connected loopback socket pair for integration tests.
 *
 * Usage:
 *   SocketPair sp;
 *   if (!sp.open()) { ... }   // binds on 127.0.0.1:0, connects, accepts
 *   net_send_raw(sp.client(), ...);
 *   net_recv_raw(sp.server_conn(), ...);
 *   sp.close();
 */

#pragma once
#include "../../network.hpp"
#include <string>
#include <cstring>

class SocketPair {
public:
    SocketPair() : m_listen(INVALID_SOCK),
                   m_client(INVALID_SOCK),
                   m_server_conn(INVALID_SOCK),
                   m_port(0) {}

    ~SocketPair() { close(); }

    /// Bind a listen socket on a random free port, connect a client, accept.
    bool open(int port_hint = 0) {
        // Bind listener on 127.0.0.1 with a random port (port=0 lets OS pick)
        m_listen = net_listen("127.0.0.1", port_hint, 1);
        if (m_listen == INVALID_SOCK) return false;

        // Discover the port the OS assigned
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getsockname(m_listen, (struct sockaddr*)&addr, &len) != 0) {
            close();
            return false;
        }
        m_port = ntohs(addr.sin_port);

        // Connect from the client side
        m_client = net_connect("127.0.0.1", m_port, 5);
        if (m_client == INVALID_SOCK) { close(); return false; }

        // Accept on the server side
        std::string peer_ip;
        m_server_conn = net_accept(m_listen, peer_ip);
        if (m_server_conn == INVALID_SOCK) { close(); return false; }

        return true;
    }

    void close() {
        if (m_server_conn != INVALID_SOCK) { sock_close(m_server_conn); m_server_conn = INVALID_SOCK; }
        if (m_client      != INVALID_SOCK) { sock_close(m_client);      m_client      = INVALID_SOCK; }
        if (m_listen      != INVALID_SOCK) { sock_close(m_listen);      m_listen      = INVALID_SOCK; }
    }

    sock_t client()      const { return m_client; }
    sock_t server_conn() const { return m_server_conn; }
    int    port()        const { return m_port; }

private:
#ifdef _WIN32
    // getsockname is in winsock2.h — already included via network.hpp
    typedef int socklen_t;
#endif
    sock_t m_listen;
    sock_t m_client;
    sock_t m_server_conn;
    int    m_port;
};
