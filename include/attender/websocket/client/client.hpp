#pragma once

#include <attender/websocket/websocket_client_base.hpp>

namespace attender::websocket
{
    class client
        : public client_base<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>
    {
    public:
        // keep this class stateless or threadsafety will be void
        using client_base<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>::client_base;

        /**
         * Connect to a remote websocket server.
         */
        boost::system::error_code connect_sync(connection_parameters const& params) override
        {
            return std::visit(overloaded{
                [&params, impl_{this->impl_}](boost::asio::ip::tcp::resolver::results_type const& resolved)
                {
                    boost::system::error_code ec;
                    
                    impl_->endpoint_ = boost::asio::connect(impl_->ws_.next_layer(), resolved, ec);
                    if (ec)
                        return ec;

                    impl_->ws_.handshake(params.host + ':' + std::to_string(impl_->endpoint_.port()), params.target, ec);
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
        void connect(connection_parameters const& params, std::function <void(const boost::system::error_code&)> const& on_completion)
        {
            std::visit(overloaded{
                [impl_{this->impl_}, &params, &on_completion](auto const& resolved)
                {
                    boost::asio::async_connect(impl_->ws_.next_layer(), resolved, [impl_, params, on_completion](
                        boost::system::error_code ec,
                        const boost::asio::ip::tcp::endpoint& endpoint
                    )
                    {
                        if (ec)
                            return on_completion(ec);

                        impl_->endpoint_= endpoint;
                        impl_->set_user_agent();
                        impl_->ws_.handshake(params.host + ':' + std::to_string(impl_->endpoint_.port()), params.target, ec);

                        on_completion(ec);
                    });
                }, 
                [on_completion](boost::system::error_code ec) 
                {
                    on_completion(ec);
                }}, 
                resolve(params)
            );
        }
    };
}