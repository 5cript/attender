#include "response.hpp"

namespace attender
{
    response_handler::response_handler(std::shared_ptr <tcp_connection> connection)
        : connection_(connection)
    {

    }
}
