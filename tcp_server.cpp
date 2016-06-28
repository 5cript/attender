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
        // This destructor does a lot of work and ist extremely important overall.
        // The stop action and all ensuing destructors perform a close on all sockets and a cleanup.
        //
        // The whole library bases on the assumption (among others) that a
        // connection will never ever outlive the server instance itself.
        //
        // This is a very reasonable assumption as there is no conceptual sense in having dangling connections.
        // This will ensure to close/abort every remaining connection and stop them from accessing
        // dangling pointers to the server or connection manager.
        //
        // stop() => stops the acceptor.
        // ~connections_() => deleting the connection manager closes all connections
        // ~socket_ => closes the server socket. The system can now free the port and handles.
    }
//---------------------------------------------------------------------------------------------------------------------
    connection_manager* tcp_server::get_connections()
    {
        return &connections_;
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
                                if (on_missing_handler_)
                                    on_missing_handler_(req, res);
                                else
                                {
                                    res->send_status(404);
                                }
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
