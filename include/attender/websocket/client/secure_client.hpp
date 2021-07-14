#pragma once

#include <attender/websocket/websocket_client_base.hpp>
#include <attender/ssl_contexts/ssl_context_interface.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/ssl.hpp>


#include <iostream>

namespace attender::websocket
{
    class secure_client
        : public client_base<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>
    {
    public:
        // keep this class stateless or threadsafety will be void
        secure_client(asio::io_service* service, std::unique_ptr <ssl_context_interface>&& context)
            : client_base(service, std::move(context))
        {
        }

        boost::system::error_code connect_sync(connection_parameters const& params) override
        {
            return std::visit(overloaded{
                [&params, impl_{this->impl_}](boost::asio::ip::tcp::resolver::results_type const& resolved)
                {
                    boost::system::error_code ec;
                    
                    impl_->endpoint_ = boost::beast::get_lowest_layer(impl_->ws_).connect(resolved, ec);
                    if (ec)
                        return ec;

                    const auto hostWithPort = params.host + ':' + std::to_string(impl_->endpoint_.port());

                    ec = impl_->setTlsHostname(hostWithPort.c_str());
                    if (ec)
                        return ec;

                    impl_->ws_.next_layer().handshake(boost::asio::ssl::stream_base::client, ec);
                    if (ec)
                        return ec; 

                    impl_->ws_.handshake(hostWithPort, params.target, ec);
                    return ec;
                }, 
                [](boost::system::error_code ec) 
                {
                    return ec;
                }}, 
                resolve(params)
            );
        }

        /**
         * Connect to a remote websocket server.
         */
        void connect(connection_parameters const& params, std::function <void(const boost::system::error_code&, char const*)> const& on_completion)
        {
            std::visit(overloaded{
                [impl_{this->impl_}, &params, &on_completion](auto const& resolved)
                {
                    boost::beast::get_lowest_layer(impl_->ws_).expires_after(params.secureAsyncOpsExpiration);
                    boost::beast::get_lowest_layer(impl_->ws_).async_connect(resolved, [impl_, params, on_completion](
                        boost::system::error_code ec,
                        const boost::asio::ip::tcp::endpoint& endpoint
                    )
                    {
                        if (ec)
                            return on_completion(ec, "async_connect");
                                                              
                        impl_->endpoint_= endpoint;  

                        const auto hostWithPort = params.host + ':' + std::to_string(impl_->endpoint_.port());

                        ec = impl_->setTlsHostname(hostWithPort.c_str());
                        if (ec)
                            return on_completion(ec, "async_connect"); 

                        impl_->ws_.next_layer().handshake(boost::asio::ssl::stream_base::client, ec);
                        if (ec)
                            return on_completion(ec, "ssl handshake"); 

                        impl_->set_user_agent();
                        impl_->ws_.handshake(hostWithPort, params.target, ec);
                        on_completion(ec, "handshake"); 
                        /*
                        boost::beast::get_lowest_layer(impl_->ws_).expires_after(params.secureAsyncOpsExpiration);
                        impl_->ws_.next_layer().async_handshake(
                            boost::asio::ssl::stream_base::client,
                            [impl_, on_completion, params](boost::system::error_code ec)
                            {
                                std::cout << "not done\n";
                                if (ec)
                                    return on_completion(ec, "async ssl handshake");  

                                boost::beast::get_lowest_layer(impl_->ws_).expires_never(); 

                                impl_->ws_.set_option(
                                    boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client)
                                );
                                
                                impl_->set_user_agent();

                                impl_->ws_.async_handshake(params.host + ':' + std::to_string(impl_->endpoint_.port()), params.target, [on_completion](boost::system::error_code ec){
                                    std::cout << "done\n";
                                    on_completion(ec, "async ws handshake");
                                });
                            }
                        );
                        */
                    });
                }, 
                [on_completion](boost::system::error_code ec) 
                {
                    on_completion(ec, "resolve");
                }}, 
                resolve(params)
            );
        }
    };
}