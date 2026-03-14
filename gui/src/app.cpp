/**
 * @file app.cpp
 * @brief Per-frame event draining from GuiClient and GuiServer adapters.
 *
 * app_poll_events() is called once per frame from main_gui.cpp before
 * any ImGui drawing.  It drains both adapter event queues and translates
 * events into AppState updates (new messages, peer list refresh, etc.).
 */

#include "app.hpp"

void app_poll_events(AppState& app) {
    // ── Client events ─────────────────────────────────────────────────────────
    if (app.screen == AppScreen::Client) {
        GuiClientEvent ev;
        while (app.gui_client.poll(ev)) {
            switch (ev.type) {
            case GuiClientEvent::Type::Message:
                app.push_message(ev.sender, ev.text, ev.is_private, false);
                break;

            case GuiClientEvent::Type::SystemMsg:
                app.push_message("*", ev.text, false, true);
                break;

            case GuiClientEvent::Type::PeerListUpdate:
                // Peer list is read directly via gui_client.peers() each frame
                break;

            case GuiClientEvent::Type::Disconnected:
                app.screen = AppScreen::Connect;
                app.set_status("Disconnected from server.", true);
                break;

            case GuiClientEvent::Type::FileRequest: {
                FileTransferEntry fte;
                fte.filename   = ev.text;
                fte.peer       = ev.sender;
                fte.total_bytes = 0;
                fte.bytes_done  = 0;
                fte.is_upload   = false;
                fte.finished    = false;
                fte.rejected    = false;
                app.transfers.push_back(fte);
                app.push_message("*",
                    "Incoming file: " + ev.text + " from " + ev.sender,
                    false, true);
                break;
            }

            case GuiClientEvent::Type::FileProgress:
                // Find the matching transfer by filename and update bytes_done
                for (auto& t : app.transfers) {
                    if (!t.finished && !t.rejected && t.filename == ev.text) {
                        t.bytes_done = ev.bytes;
                        break;
                    }
                }
                break;

            case GuiClientEvent::Type::FileComplete:
                for (auto& t : app.transfers) {
                    if (!t.finished && t.filename == ev.text) {
                        t.finished    = true;
                        t.bytes_done  = t.total_bytes;
                    }
                }
                break;

            case GuiClientEvent::Type::FileRejected:
                for (auto& t : app.transfers) {
                    if (!t.finished && t.filename == ev.text)
                        t.rejected = true;
                }
                break;
            }
        }

        // Detect unexpected disconnect
        if (!app.gui_client.is_connected() && app.screen == AppScreen::Client) {
            app.screen = AppScreen::Connect;
            app.set_status("Connection lost.", true);
        }
    }

    // ── Server events ─────────────────────────────────────────────────────────
    if (app.screen == AppScreen::Server) {
        GuiServerEvent ev;
        while (app.gui_server.poll(ev)) {
            switch (ev.type) {
            case GuiServerEvent::Type::ClientConnected:
                app.push_message("*",
                    ev.nickname + " [" + std::to_string(ev.client_id) +
                    "] connected from " + ev.ip,
                    false, true);
                break;

            case GuiServerEvent::Type::ClientDisconnected:
                app.push_message("*",
                    ev.nickname + " [" + std::to_string(ev.client_id) + "] disconnected",
                    false, true);
                break;

            case GuiServerEvent::Type::Message:
                app.push_message(ev.nickname, ev.text, ev.is_private, false);
                break;

            case GuiServerEvent::Type::FileTransferStart: {
                FileTransferEntry fte;
                fte.filename    = ev.text;
                fte.peer        = ev.nickname;
                fte.total_bytes = 0;
                fte.bytes_done  = 0;
                fte.is_upload   = false;
                fte.finished    = false;
                fte.rejected    = false;
                app.transfers.push_back(fte);
                break;
            }

            case GuiServerEvent::Type::FileTransferEnd:
                for (auto& t : app.transfers) {
                    if (!t.finished && t.filename == ev.text)
                        t.finished = true;
                }
                break;

            case GuiServerEvent::Type::AuthFailure:
                app.push_message("*",
                    "Auth failure from " + ev.ip, false, true);
                break;

            case GuiServerEvent::Type::Error:
                app.set_status("[server error] " + ev.text, true);
                break;
            }
        }
    }
}

// ── GuiClient stub implementations ───────────────────────────────────────────
// These are declared in gui_client.hpp and defined here for brevity.
// In a full build, move them to gui_client.cpp.

#include "gui_client.hpp"

GuiClient::GuiClient() {}
GuiClient::~GuiClient() { disconnect(); }

bool GuiClient::connect(const std::string& host, int port) {
    // Delegate to the underlying Client
    return m_client.connect(host, port);
}

void GuiClient::disconnect() {
    // m_client destructor handles cleanup
}

bool GuiClient::is_connected() const {
    return m_client.is_connected();
}

void GuiClient::send_broadcast(const std::string& text) {
    m_client.send_broadcast(text);
}

void GuiClient::send_private(uint32_t peer_id, const std::string& text) {
    m_client.send_private(peer_id, text);
}

void GuiClient::send_file(uint32_t peer_id, const std::string& path) {
    m_client.send_file(peer_id, path);
}

void GuiClient::send_nick_change(const std::string& new_nick) {
    m_client.send_nick_change(new_nick);
}

bool GuiClient::poll(GuiClientEvent& ev) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_queue.empty()) return false;
    ev = m_queue.front();
    m_queue.pop();
    return true;
}

std::vector<PeerInfo> GuiClient::peers() const {
    return m_client.get_peers();
}

uint32_t GuiClient::my_id() const {
    return m_client.get_my_id();
}

void GuiClient::push_event(GuiClientEvent ev) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_queue.push(std::move(ev));
}

// ── GuiServer stub implementations ───────────────────────────────────────────
#include "gui_server.hpp"

GuiServer::GuiServer() {}
GuiServer::~GuiServer() { stop(); }

bool GuiServer::start(const std::string& host, int port) {
    return m_server.start(host, port);
}

void GuiServer::stop() {
    m_server.stop();
}

bool GuiServer::is_running() const {
    return m_server.is_running();
}

void GuiServer::broadcast(const std::string& text) {
    m_server.broadcast(text);
}

void GuiServer::private_msg(uint32_t client_id, const std::string& text) {
    m_server.send_private(client_id, text);
}

void GuiServer::kick(uint32_t client_id, const std::string& reason) {
    m_server.kick_client(client_id, reason);
}

void GuiServer::send_file_to_client(uint32_t client_id, const std::string& path) {
    m_server.send_file(client_id, path);
}

void GuiServer::send_file_to_all(const std::string& path) {
    m_server.send_file_all(path);
}

bool GuiServer::poll(GuiServerEvent& ev) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_queue.empty()) return false;
    ev = m_queue.front();
    m_queue.pop();
    return true;
}

std::vector<GuiServerClientInfo> GuiServer::client_list() const {
    std::vector<GuiServerClientInfo> out;
    for (auto& ci : m_server.get_clients()) {
        GuiServerClientInfo g;
        g.id           = ci.id;
        g.nickname     = ci.nickname;
        g.ip           = ci.ip;
        g.connected_at = ci.connected_at;
        out.push_back(g);
    }
    return out;
}

size_t GuiServer::client_count() const {
    return m_server.client_count();
}

void GuiServer::push_event(GuiServerEvent ev) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_queue.push(std::move(ev));
}
