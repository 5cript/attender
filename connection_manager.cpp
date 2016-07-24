#include "connection_manager.hpp"
#include "tcp_connection.hpp"

namespace attender
{
//#####################################################################################################################
    void freeConnection(tcp_connection_interface* connection)
    {
        connection->shutdown();
        connection->stop();
        delete connection;
    }
//#####################################################################################################################
    void connection_manager::remove(tcp_connection_interface* connection)
    {
        {
            std::lock_guard <std::mutex> guard (connectionsLock_);
            connections_.erase(connection);
        }
        freeConnection(connection);
    }
//---------------------------------------------------------------------------------------------------------------------
    connection_manager::~connection_manager()
    {
        std::lock_guard <std::mutex> guard (connectionsLock_);

        for (auto& c : connections_)
            freeConnection(c);
        connections_.clear();
    }
//#####################################################################################################################
}
