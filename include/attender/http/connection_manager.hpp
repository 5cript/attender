#pragma once

#include <attender/net_core.hpp>
#include <attender/http/http_connection_interface.hpp>
#include <attender/http/http_server_interface.hpp>

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
        http_connection_interface* create(http_server_interface* server, SocketT* socket)
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
        http_connection_interface* create(http_server_interface* server, SocketT&& socket)
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
        void remove(http_connection_interface* connection);

        /**
         *  @return Returns the amount of active connections.
         */
        std::size_t count() const;

    private:
        std::unordered_set <http_connection_interface*> connections_;
        std::mutex connectionsLock_;
    };
}
