#pragma once

namespace attender
{
    struct tcp_connection_interface
    {
        virtual void start() = 0;
        virtual void stop() = 0;
    };
}
