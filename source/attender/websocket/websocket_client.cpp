#include <attender/websocket/websocket_client.hpp>
#include <attender/utility/visit_overloaded.hpp>

#include <boost/beast/core/buffers_to_string.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/buffer.hpp>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace beast_websocket = boost::beast::websocket;

namespace attender::websocket
{
    struct client::implementation : public std::enable_shared_from_this<client::implementation>
    {
        asio::io_service* service_;
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
        boost::asio::ip::tcp::endpoint endpoint_;
        boost::beast::flat_buffer read_buffer_;
        std::function<void(boost::system::error_code, std::string const&)> read_cb_;

        void set_user_agent()
        {
            ws_.set_option(beast_websocket::stream_base::decorator{
                [](beast_websocket::request_type& req)
                {
                    req.set(http::field::user_agent, std::string{BOOST_BEAST_VERSION_STRING} + " attender_wsc");
                }
            });
        }

        void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            if (ec)
                return read_cb_(ec, "");

            read_cb_(ec, boost::beast::buffers_to_string(read_buffer_.data()));
            read_buffer_.consume(bytes_transferred);

            ws_.async_read(read_buffer_, [impl_{shared_from_this()}](auto&&... args){
                impl_->on_read(std::forward<decltype(args)>(args)...);            
            });
        }

        implementation(asio::io_service* service)
            : service_{service}
            , ws_{*service}
            , endpoint_{}
            , read_buffer_{}
        {}
    };

    client::client(asio::io_service* service)
        : impl_{make_shared<client::implementation>(service)}
    {
    }

    client::~client()
    {
        // there should never be an exception, but lets not std::terminate by accident.
        try
        {
            disconnect();
        }
        catch(...)
        {
        }
    }

    void client::set_timeout(timeouts const& timeouts)
    {
        beast_websocket::stream_base::timeout opt{
            timeouts.handshake_timeout ? beast_websocket::stream_base::none() : *timeouts.handshake_timeout,
            timeouts.idle_timeout ? beast_websocket::stream_base::none() : *timeouts.idle_timeout,
            false
        };

        impl_->ws_.set_option(opt);
    }

    boost::system::error_code client::connect_sync(connection_parameters const& params)
    {
        return std::visit(overloaded{
            [&params, this](boost::asio::ip::tcp::resolver::results_type const& resolved)
            {
                boost::system::error_code ec;
                impl_->endpoint_ = boost::asio::connect(impl_->ws_.next_layer(), resolved, ec);
                if (ec)
                    return ec;

                impl_->set_user_agent();
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

    std::variant<boost::asio::ip::tcp::resolver::results_type, boost::system::error_code> client::resolve(connection_parameters const& params) const
    {
        boost::system::error_code ec;
        tcp::resolver resolver{*impl_->service_};
        auto results = resolver.resolve(params.host, params.port, ec);
        if (ec)
            return ec;
        return results;
    }

    void client::connect(connection_parameters const& params, std::function <void(const boost::system::error_code&)> const& on_completion)
    {
        std::visit(overloaded{
            [this, &params, &on_completion](auto const& resolved)
            {
                boost::asio::async_connect(impl_->ws_.next_layer(), resolved, [impl_{this->impl_}, params, on_completion](
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

    boost::system::error_code client::disconnect()
    {
        boost::system::error_code ec;
        if (impl_->ws_.is_open())
            impl_->ws_.close(beast_websocket::close_code::normal, ec);
        return ec;
    }

    void client::write_sync(std::string const& data)
    {
        impl_->ws_.write(boost::asio::buffer(data));
    }

    void client::write(std::string const& data, std::function <void(const boost::system::error_code&, std::size_t)> const& on_completion)
    {
        std::shared_ptr <const std::string> buf = std::make_shared <const std::string>(data);
        impl_->ws_.async_write(boost::asio::buffer(buf->data(), buf->size()), [buf, on_completion]
        (
            boost::system::error_code const& ec,
            std::size_t amountWritten
        )
        {
            on_completion(ec, amountWritten);
        });
    }

    void client::listen(std::function<void(boost::system::error_code, std::string const&)> read_cb)
    {
        impl_->read_cb_ = std::move(read_cb);
        impl_->ws_.async_read(impl_->read_buffer_, [impl_{this->impl_}](auto&&... args){
            impl_->on_read(std::forward<decltype(args)>(args)...);
        });
    }

    beast_websocket::stream<boost::asio::ip::tcp::socket>& client::lower_layer()
    {
        return impl_->ws_;
    }
}