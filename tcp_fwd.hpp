#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace attender
{
    class tcp_connection;
    class tcp_server;
    class tcp_stream_device;

    using read_handler = std::function <void(tcp_stream_device&& /*stream*/)>;
    using write_handler = std::function <void(boost::system::error_code /*ec*/, std::shared_ptr <tcp_connection> /*connection*/)>;
    using accept_handler = std::function <bool(boost::asio::ip::tcp::socket* /*socket*/)>;
}
