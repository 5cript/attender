#include <attender/http/http_server.hpp>
#include <attender/http/http_connection.hpp>

#include <attender/http/request.hpp>
#include <attender/http/response.hpp>

#include <iostream>

namespace attender
{
//#####################################################################################################################
    http_server::http_server(
        asio::io_service* service,
        error_callback on_error,
        settings setting
    )
        : http_basic_server(service, std::move(on_error), std::move(setting))
        , socket_{*service}
        , on_accept_{[](boost::asio::ip::tcp::socket const&){return true;}}
        , on_connection_timeout_{}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    http_server::http_server(
        asio::io_service* service,
        error_callback on_error,
        final_callback on_connection_timeout,
        settings setting
    )
        : http_basic_server(service, std::move(on_error), std::move(setting))
        , socket_{*service}
        , on_accept_{[](boost::asio::ip::tcp::socket const&){return true;}}
        , on_connection_timeout_{on_connection_timeout}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_server::add_accept_handler(accept_callback <boost::asio::ip::tcp::socket> const& on_accept)
    {
        on_accept_ = on_accept;
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_server::do_accept()
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
                    auto* connection = connections_.create <http_connection> (this, std::move(this->socket_), on_connection_timeout_);

                    auto* res = new response_handler (connection); // noexcept
                    auto* req = new request_handler (connection); // noexcept

                    static_cast <http_connection*> (connection)->attach_lifetime_binder(new lifetime_binding (req, res));

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
