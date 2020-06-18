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
    void tcp_secure_server::add_accept_handler(accept_callback <socket_type> const& on_accept)
    {
        on_accept_ = on_accept;
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_secure_server::do_accept()
    {
        socket_.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(*service_, *context_->get_ssl_context()));
        acceptor_.async_accept(internal::get_socket_layer(*socket_),
            [this](boost::system::error_code ec)
            {
                // The operation was aborted. This most likely means, that the connection was terminated.
                // Accessing this from here is unsafe.
                if (ec == boost::asio::error::operation_aborted)
                    return;

                if (!acceptor_.is_open())
                    return;

                if (!ec && on_accept_(*socket_))
                {
                    auto* connection = connections_.create <tcp_secure_connection> (this, socket_.release());

                    static_cast <tcp_secure_connection*> (connection)->get_secure_socket()->async_handshake(
                        boost::asio::ssl::stream_base::server,
                        [&, connection, this](boost::system::error_code const& ec)
                        {
                            // handshake done
                            if (!ec)
                            {
                                // handshake completed successfully

                                auto* res = new response_handler (connection); // noexcept
                                auto* req = new request_handler (connection); // noexcept

                                static_cast <tcp_secure_connection*> (connection)->attach_lifetime_binder(new lifetime_binding (req, res));

                                req->initiate_header_read(
                                    [this, res, req, connection](boost::system::error_code ec, std::exception const& exc)
                                    {
                                        auto clearConnection = [this, connection]()
                                        {
                                            connections_.remove(connection);
                                        };

                                        // socket closed
                                        if (ec.value() == 2)
                                            return clearConnection();

                                        if (ec == boost::asio::error::operation_aborted)
                                            return clearConnection();

                                        // some other error
                                        if (ec)
                                        {
                                            on_error_(connection, ec, exc);
                                            if (ec.value() == boost::system::errc::protocol_error)
                                                connection->get_response_handler().send_status(400);
                                            else
                                                connections_.remove(connection);
                                            return;
                                        }

                                        // finished header parsing.
                                        match_result best_match;
                                        auto maybeRoute = router_.find_route(req->get_header(), best_match);
                                        if (maybeRoute)
                                        {
                                            req->set_parameters(maybeRoute.get().get_path_parameters(req->get_header().get_path()));
                                            if (handle_session(req, res))
                                                maybeRoute.get().get_callback()(req, res);
                                        }
                                        else
                                        {
                                            if (best_match == match_result::path_match)
                                                res->send_status(405);
                                            else if (on_missing_handler_)
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
                                on_error_(nullptr, ec, {});
                            }
                        }
                    ); // async handshake
                }
                else if (ec)
                {
                    on_error_(nullptr, ec, {});
                }

                do_accept();
            }
        );
    }
//#####################################################################################################################
}
