#pragma once

#include "tcp_fwd.hpp"
#include "request_header.hpp"

namespace attender
{
    class request_handler
    {
    public:
        request_handler(std::shared_ptr <tcp_connection> connection);

    private:
        // read handlers
        void header_read_handler(boost::system::error_code ec);

    private:
        std::string header_buffer_;
        request_header header_;
        std::shared_ptr <tcp_connection> connection_;
    };
}
