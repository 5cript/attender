#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace attender
{
    class tcp_connection;
    class tcp_server;
    class tcp_stream_device;
    class request_handler;
    class response_handler;
    class request_header;
    class request_parser;

    using read_callback = std::function <void(boost::system::error_code /* ec */)>;
    using write_callback = std::function <void(boost::system::error_code /* ec */, std::shared_ptr <tcp_connection> /* connection */)>;
    using accept_callback = std::function <bool(boost::asio::ip::tcp::socket const& /* sock */)>;
    using connected_callback = std::function <void(std::shared_ptr <request_handler> request,
                                                   std::shared_ptr <response_handler> response)>;
    using parse_callback = std::function <void(request_header header)>;
}
