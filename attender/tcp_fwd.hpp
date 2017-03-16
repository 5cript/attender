#pragma once

#include <boost/asio.hpp>
#include <memory>

#ifdef DELETE
#   undef DELETE
#endif // DELETE

namespace attender
{
    class connection_manager;

    class tcp_server_interface;
    class tcp_server;
    class tcp_basic_server;
    class tcp_secure_server;

    class ssl_context_interface;

    class tcp_stream_device;
    class tcp_read_sink;

    class tcp_connection_interface;
    class tcp_connection;

    class request_header;
    class request_parser;
    class request_handler;

    class response_handler;
    class response_header;
    class mount_response;

    // callback for functions with error code
    using custom_callback = std::function <void(boost::system::error_code /* ec */)>;
    using read_callback = custom_callback;
    using write_callback = custom_callback;
    using parse_callback = custom_callback;
    using error_callback = std::function <void(tcp_connection_interface* /*connection*/, boost::system::error_code /*ec*/)>;

    // request finalizing callback
    using final_callback = std::function <void(request_handler* /*request*/,
                                               response_handler* /*response*/)>;
    using connected_callback = final_callback;
    using missing_handler_callback = final_callback;
    using interactive_connected_callback = std::function <bool(request_handler* /*request*/,
                                                               response_handler* /*response*/)>;
    using mount_callback = std::function <bool(request_handler* request,
                                               mount_response* mount_response)>;

    // accept callback
    template <typename SocketT>
    using accept_callback = std::function <bool(SocketT const& /* sock */)>;

    constexpr auto nop = []{};
}
