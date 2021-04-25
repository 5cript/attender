#pragma once

#include <attender/http/http_connection_base.hpp>

namespace attender
{
    /**
     *  An unsecure tcp connection.
     */
    class http_connection : public http_connection_base <asio::ip::tcp::socket>
    {
    public:
        explicit http_connection(http_server_interface* parent, boost::asio::ip::tcp::socket&& socket);
        ~http_connection() = default;

        boost::system::error_code wait_write() override
        {
            boost::system::error_code ec;
            socket_->wait(boost::asio::ip::tcp::socket::wait_write, ec);
            return ec;
        }
        boost::system::error_code wait_read() override
        {
            boost::system::error_code ec;
            socket_->wait(boost::asio::ip::tcp::socket::wait_read, ec);
            return ec;
        }

        void shutdown() override;
    };
}
