#pragma once

#include "net_core.hpp"
#include "tcp_fwd.hpp"
#include "tcp_server_interface.hpp"
#include "connection_manager.hpp"

namespace attender
{
    class tcp_server : public tcp_server_interface
    {
    public:
        tcp_server(asio::io_service* service,
                   read_handler on_read,
                   accept_handler on_accept);
        ~tcp_server();

        void start(std::string const& port, std::string const& host = "0.0.0.0") override;
        void stop() override;

    private:
        void do_accept();

    private:
        asio::io_service* service_;
        connection_manager connections_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::ip::tcp::endpoint local_endpoint_;
        read_handler on_read_;
        accept_handler on_accept_;
    };
}
