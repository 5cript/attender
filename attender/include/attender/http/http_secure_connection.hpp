#pragma once

#include <attender/http/http_connection_base.hpp>

#include <boost/asio/ssl.hpp>

#include <iostream>

namespace attender
{
    /**
     *  A secure tcp connection.
     */
    class http_secure_connection : public http_connection_base <
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
    >
    {
    public:
        using ssl_socket_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    public:
        explicit http_secure_connection(http_server_interface* parent, ssl_socket_type* socket);
        ~http_secure_connection()
        {
            //std::cout << "secure connection died\n";
        }

        boost::system::error_code wait_write() override
        {
            boost::system::error_code ec;
            socket_->lowest_layer().wait(boost::asio::ip::tcp::socket::wait_write, ec);
            return ec;
        }
        boost::system::error_code wait_read() override
        {
            boost::system::error_code ec;
            socket_->lowest_layer().wait(boost::asio::ip::tcp::socket::wait_read, ec);
            return ec;
        }

        void shutdown() override;

        ssl_socket_type* get_secure_socket();

    private:

    };
}
