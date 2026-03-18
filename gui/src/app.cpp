#include "../include/app.hpp"
#include <cstring>

AppState::AppState()
    : screen(AppScreen::Connect)
    , status_is_error(false)
    , cs_port(9000)
    , cs_encrypt(0)
    , cs_require_key(false)
    , cs_timeout_sec(10)
    , cs_auto_reconnect(false)
    , dark_theme(true)
    , show_about(false)
    , show_help(false)
{
    memset(cs_host,         0, sizeof(cs_host));
    memset(cs_nick,         0, sizeof(cs_nick));
    memset(cs_key,          0, sizeof(cs_key));
    memset(cs_access_key,   0, sizeof(cs_access_key));
    memset(cs_profile_name, 0, sizeof(cs_profile_name));

    strncpy(cs_host,         "127.0.0.1", sizeof(cs_host) - 1);
    strncpy(cs_nick,         "anonymous", sizeof(cs_nick) - 1);
    strncpy(cs_profile_name, "default",   sizeof(cs_profile_name) - 1);
}

void AppState::push_message(const std::string& sender, const std::string& text,
                             bool is_private, bool is_system)
{
    if (messages.size() >= MAX_MESSAGES)
        messages.pop_front();
    messages.push_back(ChatMessage(sender, text, is_private, is_system));
}

void AppState::set_status(const std::string& msg, bool error)
{
    status_msg      = msg;
    status_is_error = error;
    log(msg);
}

void AppState::log(const std::string& msg)
{
    if (conn_log.size() >= MAX_LOG)
        conn_log.pop_front();
    conn_log.push_back(msg);
}
