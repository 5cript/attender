#pragma once

#include <attender/net_core.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <string_view>
#include <mutex>

namespace attender::websocket
{

class connection;

class session_base
{
public:
    session_base(connection* owner);

    virtual void on_close() = 0;
    virtual void on_text(std::string_view data) = 0;
    virtual void on_binary(char const* begin, std::size_t amount) = 0;
    virtual void on_error(boost::system::error_code ec, char const* where) = 0;
    virtual void on_write_complete(std::size_t bytes_transferred);
    
    bool write_text(std::string_view text, std::function<void(std::size_t)> const& on_complete  = [](auto){});
    bool write_binary(char const* data, std::size_t amount, std::function<void(std::size_t)> const& on_complete = [](auto){});

    virtual ~session_base() = default;

private:
    bool write_common(char const* begin, std::size_t amount, std::function<void(std::size_t)> const& on_complete);

private:
    connection* owner_;
    std::function<void(std::size_t)> on_write_complete_;
};

}