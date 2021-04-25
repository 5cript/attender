#pragma once

#include <attender/net_core.hpp>
#include <attender/http/http_fwd.hpp>
#include <attender/http/http_basic_server.hpp>

namespace attender
{
    /**
     *  An insecure server.
     */
    class http_server : public http_basic_server
    {
    public:
        http_server(asio::io_service* service,
                   error_callback on_error,
                   settings setting = {});
        ~http_server() = default;

        /**
         *  Add handler that is called when an incomming connection is accepted.
         *  Return false here in the handler to not continue with request handling.
         */
        void add_accept_handler(accept_callback <boost::asio::ip::tcp::socket> const& on_accept);

    protected:
        void do_accept() override;

    private:
        boost::asio::ip::tcp::socket socket_;
        accept_callback <boost::asio::ip::tcp::socket> on_accept_;
    };
}
