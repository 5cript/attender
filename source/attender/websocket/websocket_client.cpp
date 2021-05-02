#include <attender/websocket/websocket_client.hpp>

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
    {
    }

    client::~client()
    {
        disconnect();
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
        std::string host = params.host;

        boost::system::error_code ec;

        tcp::resolver resolver{*service_};
        auto const results = resolver.resolve(host, params.port, ec);
        if (!ec)
            return ec;

        auto ep = boost::asio::connect(ws_.next_layer(), results, ec);
        if (!ec)
            return ec;

        host += ':' + std::to_string(ep.port());

        ws_.set_option(beast_websocket::stream_base::decorator{
            [](beast_websocket::request_type& req)
            {
                req.set(http::field::user_agent, std::string{BOOST_BEAST_VERSION_STRING} + " attender_wsc");
            }
        });

        ws_.handshake(host, params.target, ec);
        return ec;
    }

    void client::disconnect()
    {
        ws_.close(beast_websocket::close_code::normal);
    }

    void client::write_sync(std::string const& data)
    {
        ws_.write(boost::asio::buffer(data));
    }

    beast_websocket::stream<boost::asio::ip::tcp::socket>& client::lower_layer()
    {
        return ws_;
    }
}