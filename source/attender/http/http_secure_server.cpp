#include <attender/http/http_secure_server.hpp>
#include <attender/ssl_contexts/ssl_context_interface.hpp>
#include <attender/http/http_secure_connection.hpp>

#include <attender/http/request.hpp>
#include <attender/http/response.hpp>

#include <iostream>

namespace attender
{
//#####################################################################################################################
    http_secure_server::http_secure_server(
        asio::io_service* service,
        std::unique_ptr <ssl_context_interface> context,
        error_callback on_error,
        settings setting
    )
        : http_basic_server(service, std::move(on_error), std::move(setting))
        , context_{std::move(context)}
        , on_accept_{[](http_secure_server::socket_type const&){return true;}}
        , on_connection_timeout_{}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    http_secure_server::http_secure_server(
        asio::io_service* service,
        std::unique_ptr <ssl_context_interface> context,
        error_callback on_error,
        final_callback on_connection_timeout,
        settings setting
    )
        : http_basic_server(service, std::move(on_error), std::move(setting))
        , context_{std::move(context)}
        , on_accept_{[](http_secure_server::socket_type const&){return true;}}
        , on_connection_timeout_{on_connection_timeout}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_secure_server::add_accept_handler(accept_callback <socket_type> const& on_accept)
    {
        on_accept_ = on_accept;
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_secure_server::do_accept()
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
                    auto* connection = connections_.create <http_secure_connection> (this, socket_.release(), on_connection_timeout_);

                    static_cast <http_secure_connection*> (connection)->get_secure_socket()->async_handshake(
                        boost::asio::ssl::stream_base::server,
                        [&, connection, this](boost::system::error_code const& ec)
                        {
                            // handshake done
                            if (!ec)
                            {
                                // handshake completed successfully

                                auto* res = new response_handler (connection); // noexcept
                                auto* req = new request_handler (connection); // noexcept

                                static_cast <http_secure_connection*> (connection)->attach_lifetime_binder(new lifetime_binding (req, res));

                                req->initiate_header_read(
                                    [this, res, req, connection](boost::system::error_code ec, std::exception const& exc)
                                    {
                                        header_read_handler(req, res, connection, ec, exc);
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
