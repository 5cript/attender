#pragma once

#include <boost/asio.hpp>
#include <string_view>
#include <memory>

#ifdef DELETE
#   undef DELETE
#endif // DELETE

namespace attender
{
    class connection_manager;

    class http_server_interface;
    class http_server;
    class http_basic_server;
    class http_secure_server;

    class ssl_context_interface;

    class http_stream_device;
    class http_read_sink;

    class http_connection_interface;
    class http_connection;

    class request_header;
    class request_parser;
    class request_handler;

    class response_handler;
    class response_header;
    class mount_response;

    // callback for functions with error code
    using custom_callback = std::function <void(boost::system::error_code /* ec */)>;
    using read_callback = std::function <void(boost::system::error_code /* ec */, std::size_t amountRead)>;
    using write_callback = std::function <void(boost::system::error_code /* ec */, std::size_t amount)>;
    using parse_callback = std::function <void(boost::system::error_code /* ec */, std::exception const& /*exc*/)>;
    using error_callback = std::function <void(http_connection_interface* /*connection*/, boost::system::error_code /*ec*/, std::exception const& /* exc */)>;

    // request finalizing callback
    using final_callback = std::function <void(request_handler* /*request*/,
                                               response_handler* /*response*/)>;
    using connected_callback = final_callback;
    using missing_handler_callback = final_callback;
    using interactive_connected_callback = std::function <bool(request_handler* /*request*/,
                                                               response_handler* /*response*/)>;

    using mount_callback = std::function <bool(request_handler* request,
                                               response_handler* mount_response)>;
    using mount_callback_2 = std::function <bool(request_handler* request,
                                                 response_handler* mount_response,
                                                 std::string_view real_path)>;

    // accept callback
    template <typename SocketT>
    using accept_callback = std::function <bool(SocketT const& /* sock */)>;

    using size_type = std::size_t;
}
