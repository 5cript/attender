#include "tcp_server.hpp"

#include "tcp_connection.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_server::tcp_server(asio::io_service* service,
                   read_handler on_read,
                   accept_handler on_accept)
        : service_{service}
        , connections_{}
        , socket_{*service}
        , acceptor_{*service}
        , on_read_{std::move(on_read)}
        , on_accept_{std::move(on_accept)}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_server::~tcp_server()
    {
        stop();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_server::start(std::string const& port, std::string const& host)
    {
        stop();

        boost::asio::ip::tcp::resolver resolver{*service_};
        local_endpoint_ = *resolver.resolve({host, port});
        acceptor_.open(local_endpoint_.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(local_endpoint_);
        acceptor_.listen();

        do_accept();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_server::stop()
    {
        acceptor_.close();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_server::do_accept()
    {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec)
            {
                if (!acceptor_.is_open())
                    return;

                if (!ec)
                {
                    on_accept_(&socket_);

                    std::cout << "new connection!\n";

                    connections_.add(std::make_shared <tcp_connection> (
                        std::move(socket_),
                        on_read_
                    ));
                }

                do_accept();
            }
        );
    }
//#####################################################################################################################
}
