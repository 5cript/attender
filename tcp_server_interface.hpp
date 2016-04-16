#pragma once

namespace attender
{
    struct tcp_server_interface
    {
        virtual ~tcp_server_interface() = default;

        virtual void start(std::string const& port, std::string const& host) = 0;
        virtual void stop() = 0;
    };
}
