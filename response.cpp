#include "response.hpp"

namespace attender
{

//#####################################################################################################################
    response_handler::response_handler(std::shared_ptr <tcp_connection> connection)
        : connection_(connection)
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::append(std::string const& field, std::string const& value)
    {
        header_.append_field(field, value);
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::set(std::string const& field, std::string const& value)
    {
        header_.set_field(field, value);
    }
//#####################################################################################################################
}
