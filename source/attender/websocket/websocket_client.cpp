#include <attender/websocket/websocket_client.hpp>

#include <boost/asio/connect.hpp>

using tcp = boost::asio::ip::tcp;

namespace attender::websocket
{
    client::client(
        asio::io_service* service
    )
        : service_{service}
        , ws_{*service}
    {
    }

    void client::connect(std::string const& port, std::string const& host = "::")
    {
        /*
        tcp::resolver resolver{*service_};
        auto const results = resolver.resolve(host, port);
        auto ep = boost::asio::connect(ws_.next_layer(), results);
        */
    }
}