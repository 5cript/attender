#include <attender/http/connection_manager.hpp>
#include <attender/http/http_connection.hpp>

#include <stdexcept>

namespace attender
{
//#####################################################################################################################
    void free_connection(http_connection_interface* connection)
    {
        connection->shutdown();
        connection->stop();
        delete connection;
    }
//#####################################################################################################################
    void connection_manager::remove(http_connection_interface* connection)
    {
        std::lock_guard <std::mutex> guard (connectionsLock_);
        if (connections_.erase(connection) != 0)
            free_connection(connection);
        else
            throw std::logic_error("connection was already freed");
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
