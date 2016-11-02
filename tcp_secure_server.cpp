#include "tcp_secure_server.hpp"
#include "ssl_context_interface.hpp"
#include "tcp_secure_connection.hpp"

#include "request.hpp"
#include "response.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_secure_server::tcp_secure_server(asio::io_service* service,
                                         std::unique_ptr <ssl_context_interface> context,
                                         error_callback on_error,
                                         settings setting)
        : tcp_basic_server(service, std::move(on_error), std::move(setting))
        , context_{std::move(context)}
        , on_accept_{[](tcp_secure_server::socket_type const&){return true;}}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_secure_server::do_accept()
    {
        socket_.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(*service_, *context_->get_ssl_context()));
        acceptor_.async_accept(internal::get_socket_layer(*socket_),
            [this](boost::system::error_code ec)
            {
                if (!acceptor_.is_open())
                    return;

                if (!ec && on_accept_(*socket_))
                {
                    socket_->async_handshake(
                        boost::asio::ssl::stream_base::server,
                        [&, this](boost::system::error_code const& ec)
                        {
                            // handshake done
                            if (!ec)
                            {
                                // handshake completed successfully
                                auto* connection = connections_.create <tcp_secure_connection> (this, socket_.release());

                                auto* res = new response_handler (connection); // noexcept
                                auto* req = new request_handler (connection); // noexcept

                                static_cast <tcp_secure_connection*> (connection)->attach_lifetime_binder(new lifetime_binding (req, res));

                                req->initiate_header_read(
                                    [this, res, req, connection](boost::system::error_code ec)
                                    {
                                        // finished header parsing.
                                        if (ec)
                                        {
                                            std::cout << "initiate read\n";
                                            on_error_(connection, ec);
                                            connections_.remove(connection);
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
                            else
                            {
                                std::cout << "nullptr\n";
                                on_error_(nullptr, ec);
                            }
                        }
                    ); // async handshake
                }
                else
                {
                    // TODO ...
                }

                do_accept();
            }
        );
    }
//#####################################################################################################################
}
