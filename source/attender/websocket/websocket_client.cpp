#include <attender/websocket/websocket_client.hpp>
#include <attender/utility/visit_overloaded.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/buffer.hpp>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace beast_websocket = boost::beast::websocket;

namespace attender::websocket
{
    client::client(asio::io_service* service)
        : service_{service}
        , ws_{*service}
        , endpoint_{}
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

        ws_.set_option(opt);
    }

    boost::system::error_code client::connect_sync(connection_parameters const& params)
    {
        return std::visit(overloaded{
            [&params, this](boost::asio::ip::tcp::resolver::results_type const& resolved)
            {
                boost::system::error_code ec;
                endpoint_ = boost::asio::connect(ws_.next_layer(), resolved, ec);
                if (ec)
                    return ec;

                set_user_agent();
                ws_.handshake(params.host + ':' + std::to_string(endpoint_.port()), params.target, ec);
                return ec;
            }, 
            [](boost::system::error_code ec) 
            {
                return ec;
            }}, 
            resolve(params)
        );
    }

    void client::set_user_agent()
    {
        ws_.set_option(beast_websocket::stream_base::decorator{
            [](beast_websocket::request_type& req)
            {
                req.set(http::field::user_agent, std::string{BOOST_BEAST_VERSION_STRING} + " attender_wsc");
            }
        });
    }

    std::variant<boost::asio::ip::tcp::resolver::results_type, boost::system::error_code> client::resolve(connection_parameters const& params) const
    {
        boost::system::error_code ec;
        tcp::resolver resolver{*service_};
        auto results = resolver.resolve(params.host, params.port, ec);
        if (ec)
            return ec;
        return results;
    }

    void client::connect(connection_parameters const& params, std::function <void(const boost::system::error_code&)> const& onCompletion)
    {
        std::visit(overloaded{
            [this, &params, &onCompletion](auto const& resolved)
            {
                boost::asio::async_connect(ws_.next_layer(), resolved, [this, params, onCompletion](
                    boost::system::error_code ec,
                    const boost::asio::ip::tcp::endpoint& endpoint
                )
                {
                    if (ec)
                        return onCompletion(ec);

                    endpoint_= endpoint;
                    set_user_agent();
                    ws_.handshake(params.host + ':' + std::to_string(endpoint_.port()), params.target, ec);

                    onCompletion(ec);
                });
            }, 
            [onCompletion](boost::system::error_code ec) 
            {
                onCompletion(ec);
            }}, 
            resolve(params)
        );
    }

    boost::system::error_code client::disconnect()
    {
        boost::system::error_code ec;
        if (ws_.is_open())
            ws_.close(beast_websocket::close_code::normal, ec);
        return ec;
    }

    void client::write_sync(std::string const& data)
    {
        ws_.write(boost::asio::buffer(data));
    }

    void client::write(std::string const& data, std::function <void(const boost::system::error_code&, std::size_t)> const& onCompletion)
    {
        std::shared_ptr <const std::string> buf = std::make_shared <const std::string>(data);
        ws_.async_write(boost::asio::buffer(buf->data(), buf->size()), [buf, onCompletion]
        (
            boost::system::error_code const& ec,
            std::size_t amountWritten
        )
        {
            onCompletion(ec, amountWritten);
        });
    }

    beast_websocket::stream<boost::asio::ip::tcp::socket>& client::lower_layer()
    {
        return ws_;
    }
}