#include "tcp_server.hpp"

#include "tcp_connection.hpp"
#include "request.hpp"
#include "response.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_server::tcp_server(asio::io_service* service,
                           error_callback on_error,
                           settings setting)
        : service_{service}
        , socket_{*service}
        , acceptor_{*service}
        , local_endpoint_{}
        , connections_{}
        , router_{}
        , settings_{std::move(setting)}
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
    settings tcp_server::get_settings() const
    {
        return settings_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_server::get(std::string path_template, connected_callback const& on_connect)
    {
        router_.add_route("GET", path_template, on_connect);
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

                    auto shared_connection = std::make_shared <tcp_connection> (this, std::move(socket_));
                    connections_.add(shared_connection);

                    auto res = std::make_shared <response_handler> (shared_connection);
                    auto req = std::make_shared <request_handler> (shared_connection);

                    shared_connection->attach_lifetime_binder(new tcp_connection::lifetime_binder {req, res});

                    req->initiate_header_read(
                        [this, res, req](boost::system::error_code ec)
                        {
                            // finished header parsing.
                            if (ec)
                            {
                                on_error_(ec);
                                return;
                            }

                            auto maybeRoute = router_.find_route(req->get_header());
                            if (maybeRoute)
                            {
                                req->set_parameters(maybeRoute.get().get_path_parameters(req->get_header().get_path()));
                                maybeRoute.get().get_callback()(req, res);
                            }
                            else
                            {
                                // TODO: 404
                            }
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
