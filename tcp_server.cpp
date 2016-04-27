#include "tcp_server.hpp"

#include "tcp_connection.hpp"
#include "request.hpp"
#include "response.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_server::tcp_server(asio::io_service* service,
                           connected_callback on_connect,
                           error_callback on_error)
        : service_{service}
        , connections_{}
        , socket_{*service}
        , acceptor_{*service}
        , on_connect_{std::move(on_connect)}
        , on_accept_{[](boost::asio::ip::tcp::socket const&){return true;}}
        , on_error_{std::move(on_error)}
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

                if (!ec && on_accept_(socket_))
                {
                    std::cout << "new connection!\n";

                    auto shared_connection = std::make_shared <tcp_connection> (std::move(socket_));
                    connections_.add(shared_connection);

                    auto res = std::make_shared <response_handler> (shared_connection);
                    auto req = std::make_shared <request_handler> (shared_connection);

                    shared_connection->attach_lifetime_binder(new tcp_connection::lifetime_binder {req, res});

                    req->initiate_header_read(
                        [this, res, req](boost::system::error_code ec)
                        {
                            if (!ec)
                                on_connect_(req, res);
                            else
                                on_error_(ec);
                        }
                    );
                }
                else if (ec)
                {
                    // TODO...
                }

                do_accept();
            }
        );
    }
//#####################################################################################################################
}
