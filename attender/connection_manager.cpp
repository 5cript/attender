#include "connection_manager.hpp"
#include "tcp_connection.hpp"

namespace attender
{
//#####################################################################################################################
    void free_connection(tcp_connection_interface* connection)
    {
        connection->shutdown();
        connection->stop();
        delete connection;
    }
//#####################################################################################################################
    void connection_manager::remove(tcp_connection_interface* connection)
    {
        std::lock_guard <std::mutex> guard (connectionsLock_);
        connections_.erase(connection);
        free_connection(connection);
    }
//---------------------------------------------------------------------------------------------------------------------
    void connection_manager::clear()
    {
        std::lock_guard <std::mutex> guard (connectionsLock_);

        for (auto& c : connections_)
            free_connection(c);
        connections_.clear();
    }
//---------------------------------------------------------------------------------------------------------------------
    connection_manager::~connection_manager()
    {
        clear();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::size_t connection_manager::count() const
    {
        return connections_.size();
    }
//#####################################################################################################################
}
