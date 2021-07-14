#pragma once

#include <attender/websocket/server/session_base.hpp>

namespace attender::websocket
{

class connection;

class noop_session : public session_base
{
public:
    noop_session(connection* con)
        : session_base{con}
    {}

    void on_close() override {}
    void on_text(std::string_view) override {}
    void on_binary(char const*, std::size_t) override {}
    void on_error(boost::system::error_code, char const*) override {}
    void on_write_complete(std::size_t) override {}

    virtual ~noop_session() = default;
};

}