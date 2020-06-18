#pragma once

#include "tcp_connection_base.hpp"

namespace attender
{
    /**
     *  An unsecure tcp connection.
     */
    class tcp_connection : public tcp_connection_base <asio::ip::tcp::socket>
    {
    public:
        explicit tcp_connection(tcp_server_interface* parent, boost::asio::ip::tcp::socket&& socket);
        ~tcp_connection() = default;

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
