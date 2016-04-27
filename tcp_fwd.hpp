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
    class tcp_read_sink;

    using custom_callback = std::function <void(boost::system::error_code /* ec */)>;

    using read_callback = custom_callback;
    using write_callback = custom_callback;
    using parse_callback = custom_callback;
    using error_callback = custom_callback;

    using accept_callback = std::function <bool(boost::asio::ip::tcp::socket const& /* sock */)>;
    using connected_callback = std::function <void(std::shared_ptr <request_handler> request,
                                                   std::shared_ptr <response_handler> response)>;
}
