#pragma once

#include "tcp_fwd.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace attender
{
    struct tcp_connection_interface
    {
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void shutdown() = 0;
        virtual void write(std::istream& stream, write_callback handler) = 0;
        virtual void write(std::string const& string, write_callback handler) = 0;
        virtual void write(std::vector <char> const& container, write_callback handler) = 0;
        virtual tcp_server_interface* get_parent() = 0;
    };
}
