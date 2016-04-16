#include "connection_manager.hpp"

namespace attender
{
//#####################################################################################################################
    void connection_manager::add(shared_connection connection)
    {
        connections_.insert(connection);
        connection->start();
    }
//---------------------------------------------------------------------------------------------------------------------
    void connection_manager::remove(shared_connection connection)
    {
        connections_.erase(connection);
        connection->stop();
    }
//---------------------------------------------------------------------------------------------------------------------
    connection_manager::~connection_manager()
    {
        for (auto& c : connections_)
            c->stop();
        connections_.clear();
    }
//#####################################################################################################################
}
