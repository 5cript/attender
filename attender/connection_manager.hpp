#pragma once

#include "net_core.hpp"
#include "tcp_connection_interface.hpp"
#include "tcp_server_interface.hpp"

#include <boost/asio.hpp>
#include <unordered_set>
#include <mutex>

namespace attender
{
    /**
     *  The connection manager holds all active connections.
     */
    class connection_manager
    {
    public:
        ~connection_manager();

        /**
         *  Add a connection to the manager.
         *
         *  @param server The owning server (which accepted the connection).
         *  @param socket The socket that is associated with the connection.
         *
         *  @return The new connection object.
         */
        template <typename T, typename SocketT>
        tcp_connection_interface* create(tcp_server_interface* server, SocketT* socket)
        {
            std::lock_guard <std::mutex> guard (connectionsLock_);

            auto c = connections_.insert(new T{server, socket});
            auto* connection = *c.first;
            connection->start();
            return connection;
        }

        /**
         *  Add a connection to the manager.
         *
         *  @param server The owning server (which accepted the connection).
         *  @param socket The socket that is associated with the connection.
         *
         *  @return The new connection object.
         */
        template <typename T, typename SocketT>
        tcp_connection_interface* create(tcp_server_interface* server, SocketT&& socket)
        {
            std::lock_guard <std::mutex> guard (connectionsLock_);

            auto c = connections_.insert(new T{server, std::move(socket)});
            auto* connection = *c.first;
            connection->start();
            return connection;
        }

        /**
         *  Remove and terminate all connections.
         */
        void clear();

        /**
         *  Remove and terminate the given connection.
         *
         *  @param connection The connection to remove.
         */
        void remove(tcp_connection_interface* connection);

        /**
         *  @return Returns the amount of active connections.
         */
        std::size_t count() const;

    private:
        std::unordered_set <tcp_connection_interface*> connections_;
        std::mutex connectionsLock_;
    };
}
