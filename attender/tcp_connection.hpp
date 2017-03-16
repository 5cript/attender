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

        void shutdown() override;
    };
}
