#pragma once

#include "tcp_fwd.hpp"

namespace attender
{
    class response_handler
    {
    public:
        response_handler(std::shared_ptr <tcp_connection> connection);

    private:
        std::shared_ptr <tcp_connection> connection_;
    };
}
