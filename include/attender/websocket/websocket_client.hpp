#pragma once

#include <attender/net_core.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace attender::websocket
{
    class client
    {
    public:
        client(
            asio::io_service* service
        );

        /**
         * Connect to a remote websocket server.
         */
        void connect(std::string const& host, std::string const& port);

    private:
        asio::io_service* service_;
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    };
}