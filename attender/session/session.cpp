#include "session.hpp"

namespace attender
{
//#####################################################################################################################
    session::session(std::string id)
        : id_{std::move(id)}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    std::string session::id() const
    {
        return id_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void session::id(std::string const& id)
    {
        id_ = id;
    }
//#####################################################################################################################
}
