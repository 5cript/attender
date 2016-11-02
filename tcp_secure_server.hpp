#pragma once

#include "net_core.hpp"
#include "tcp_fwd.hpp"
#include "tcp_basic_server.hpp"

#include <boost/asio/ssl.hpp>

namespace attender
{
    class tcp_secure_server : public tcp_basic_server
    {
    public:
        using socket_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    public:
        tcp_secure_server(asio::io_service* service,
                          std::unique_ptr <ssl_context_interface> context,
                          error_callback on_error,
                          settings setting = {});
        ~tcp_secure_server() = default;

    protected:
        void do_accept() override;

    private:
        std::unique_ptr <ssl_context_interface> context_;
        std::unique_ptr <socket_type> socket_;
        accept_callback <socket_type> on_accept_;
    };
}
