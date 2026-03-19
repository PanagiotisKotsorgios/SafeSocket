/**
 * server_gui_ext.cpp
 * Extra Server methods added for the GUI frontend.
 */
#include "../include/server.hpp"
#include "../include/network.hpp"
#include <mutex>

std::vector<Server::ClientSnapshot> Server::get_clients_snapshot()
{
    std::vector<ClientSnapshot> out;
    std::lock_guard<std::mutex> lk(m_clients_mutex);
    for (auto& kv : m_clients) {
        ClientInfo* ci = kv.second;
        if (!ci || !ci->alive) continue;
        ClientSnapshot snap;
        snap.id            = ci->id;
        snap.nickname      = ci->nickname;
        snap.ip            = ci->ip;
        snap.connected_at  = ci->connected_at;
        snap.authenticated = ci->authenticated;
        out.push_back(snap);
    }
    return out;
}

void Server::broadcast_server_message(const std::string& msg)
{
    broadcast_text(msg, SERVER_ID);
}

// ============================================================
//  GUI helper: patch Server::print() to route to callback
// ============================================================
// NOTE: We override by adding to server_gui_ext; the real print()
// in server.cpp checks m_srv_msg_cb and routes accordingly.
// This is handled via the patch below applied to server.cpp.

// ============================================================
//  GUI helper: get_stats()
// ============================================================
Server::StatsSnapshot Server::get_stats()
{
    StatsSnapshot s;
    {
        std::lock_guard<std::mutex> lk(m_clients_mutex);
        s.clients_online = m_clients.size();
    }
    s.msg_count    = m_msg_count;
    s.bytes_sent   = m_bytes_sent;
    s.bytes_recv   = m_bytes_recv;
    s.encrypt_type = encrypt_type_name(g_config.encrypt_type);
    return s;
}
