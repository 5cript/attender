#pragma once

#include <attender/net_core.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>

namespace attender::websocket
{
    class client
    {
        struct connection_parameters
        {
            std::string host = "::";
            std::string port = "";
            std::string target = "/";
        };

        struct timeouts
        {
            std::optional <std::chrono::milliseconds> handshake_timeout;
            std::optional <std::chrono::milliseconds> idle_timeout;
        };

    public:
        explicit client(asio::io_service* service);
        ~client();

        client(client const&) = delete;
        client(client&&) = default;
        client& operator=(client const&) = delete;
        client& operator=(client&&) = default;

        /**
         * Connect to a remote websocket server.
         */
        boost::system::error_code connect_sync(connection_parameters const& params);

        /**
         * Set a timeout for handshakes.
         */
        void set_timeout(timeouts const& timeouts);

        /**
         * Disconnect from the server.
         */
        void disconnect();

        /**
         * Returns the underlying boost websocket instance.
         */
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& lower_layer();

        /**
         * Writes string to the server.
         */
        void write_sync(std::string const& data);

        /**
         * Read data synchronously.
         */
        template <typename T>
        void read_sync(T& buffer)
        {
            ws_.read(buffer);
        }

    private:
        asio::io_service* service_;
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    };
}