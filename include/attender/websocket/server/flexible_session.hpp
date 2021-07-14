#pragma once

#include <attender/websocket/server/session_base.hpp>

namespace attender::websocket
{

class connection;

class flexible_session : public session_base
{
public:
    flexible_session(connection* con)
        : session_base{con}
    {}

    void on_close() override 
    {
        if (on_close_cb)
            on_close_cb();
    }
    void on_text(std::string_view view) override 
    {
        if (on_text_cb)
            on_text_cb(view);
    }
    void on_binary(char const* data, std::size_t size) override 
    {
        if (on_binary_cb)
            on_binary_cb(data, size);
    }
    void on_error(boost::system::error_code ec, char const* where) override 
    {
        if (on_error_cb)
            on_error_cb(ec, where);
    }
    void on_write_complete(std::size_t size) override 
    {
        if (on_write_complete_cb)
            on_write_complete_cb(size);
    }

    virtual ~flexible_session() = default;

public:
    std::function <void()> on_close_cb;
    std::function <void(std::string_view)> on_text_cb;
    std::function <void(char const* data, std::size_t size)> on_binary_cb;
    std::function <void(boost::system::error_code, char const*)> on_error_cb;
    std::function <void(std::size_t)> on_write_complete_cb;
};

}