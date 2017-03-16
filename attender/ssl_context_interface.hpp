#pragma once

#include "net_core.hpp"
#include "tcp_fwd.hpp"

#include <boost/asio/ssl.hpp>

#include <iostream>

namespace attender
{
    class ssl_context_interface
    {
    public:
        virtual boost::asio::ssl::context* get_ssl_context() = 0;
        virtual ~ssl_context_interface()
        {
            std::cout << "context deleted\n";
        }
    };
}
