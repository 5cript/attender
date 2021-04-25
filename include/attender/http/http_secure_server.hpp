#pragma once

#include <attender/net_core.hpp>
#include <attender/http/http_fwd.hpp>
#include <attender/http/http_basic_server.hpp>

#include <boost/asio/ssl.hpp>

namespace attender
{
    /**
     *  A secure SSL/TLS server.
     */
    class http_secure_server : public http_basic_server
    {
    public:
        using socket_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    public:
        http_secure_server(asio::io_service* service,
                          std::unique_ptr <ssl_context_interface> context,
                          error_callback on_error,
                          settings setting = {});
        ~http_secure_server() = default;

        /**
         *  Add handler that is called when an incomming connection is accepted.
         *  Return false here in the handler to not continue with request handling.
         */
        void add_accept_handler(accept_callback <socket_type> const& on_accept);

    protected:
        void do_accept() override;

    private:
        std::unique_ptr <ssl_context_interface> context_;
        std::unique_ptr <socket_type> socket_;
        accept_callback <socket_type> on_accept_;
    };
}
