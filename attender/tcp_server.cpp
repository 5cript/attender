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
    void tcp_server::add_accept_handler(accept_callback <boost::asio::ip::tcp::socket> const& on_accept)
    {
        on_accept_ = on_accept;
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
                        [this, res, req, connection](boost::system::error_code ec, std::exception const& exc)
                        {
                            header_read_handler(req, res, connection, ec, exc);
                        }
                    );
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
