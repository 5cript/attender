#pragma once

#include "net_core.hpp"
#include "tcp_connection_interface.hpp"

#include <boost/asio.hpp>
#include <unordered_set>
#include <mutex>

namespace attender
{
    class connection_manager
    {
    public:
        ~connection_manager();

        template <typename T>
        tcp_connection_interface* create(tcp_server* server, boost::asio::ip::tcp::socket&& socket)
        {
            std::lock_guard <std::mutex> guard (connectionsLock_);

            auto c = connections_.insert(new T{server, std::move(socket)});
            auto* connection = *c.first;
            connection->start();
            return connection;
        }

        void remove(tcp_connection_interface* connection);

    private:
        std::unordered_set <tcp_connection_interface*> connections_;
        std::mutex connectionsLock_;
    };
}
