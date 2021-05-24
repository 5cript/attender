#pragma once

#include <attender/net_core.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>
#include <variant>

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
            std::optional <std::chrono::milliseconds> handshake_timeout{std::nullopt};
            std::optional <std::chrono::milliseconds> idle_timeout{std::nullopt};
        };

    public:
        /**
         * Construct a client with an io_service and an error handler for errors occuring from
         * sources other than functions this class provides. For instance the destructor can cause errors.
         * 
         * @param service A boost io_service
         * @param errorHandler A function taking a cstring and an error code and a message
         */
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
         * Connect to a remote websocket server.
         */
        void connect(connection_parameters const& params, std::function <void(const boost::system::error_code&)> const& onCompletion);

        /**
         * Set a timeout for handshakes.
         */
        void set_timeout(timeouts const& timeouts);

        /**
         * Disconnect from the server.
         */
        boost::system::error_code disconnect();

        /**
         * Returns the underlying boost websocket instance.
         */
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& lower_layer();

        /**
         * Writes string to the server.
         */
        void write_sync(std::string const& data);

        /**
         * Writes string to the server.
         * @param data Data to write.
         * @param onCompletion What to do when complete.
         */
        void write(std::string const& data, std::function <void(const boost::system::error_code&, std::size_t)> const& onCompletion);

        /**
         * Read data synchronously.
         */
        template <typename T>
        void read_sync(T& buffer)
        {
            ws_.read(buffer);
        }

    private:
        std::variant<boost::asio::ip::tcp::resolver::results_type, boost::system::error_code> resolve(connection_parameters const& params) const;
        void set_user_agent();

    private:
        asio::io_service* service_;
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
        boost::asio::ip::tcp::endpoint endpoint_;
    };
}