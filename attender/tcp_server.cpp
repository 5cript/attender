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
        : tcp_basic_server(service, std::move(on_error), std::move(setting))
        , socket_{*service}
        , on_accept_{[](boost::asio::ip::tcp::socket const&){return true;}}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_server::do_accept()
    {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec)
            {
                // the operation was aborted. This usually means, that the server has been destroyed.
                // accessing this is unsafe now.
                if (ec == boost::asio::error::operation_aborted)
                    return;

                if (!acceptor_.is_open())
                    return;

                if (!ec && on_accept_(socket_))
                {
                    auto* connection = connections_.create <tcp_connection> (this, std::move(this->socket_));

                    auto* res = new response_handler (connection); // noexcept
                    auto* req = new request_handler (connection); // noexcept

                    static_cast <tcp_connection*> (connection)->attach_lifetime_binder(new lifetime_binding (req, res));

                    req->initiate_header_read(
                        [this, res, req, connection](boost::system::error_code ec)
                        {
                            // socket closed
                            if (ec.value() == 2)
                                return;

                            if (ec == boost::asio::error::operation_aborted)
                                return;

                            if (ec)
                            {
                                on_error_(connection, ec);
                                connections_.remove(connection);
                                return;
                            }

                            // finished header parsing.
                            match_result best_match;
                            auto maybeRoute = router_.find_route(req->get_header(), best_match);
                            if (maybeRoute)
                            {
                                req->set_parameters(maybeRoute.get().get_path_parameters(req->get_header().get_path()));
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
