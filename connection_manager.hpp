#pragma once

#include "net_core.hpp"
#include "tcp_connection_interface.hpp"

#include <unordered_set>

namespace attender
{
    class connection_manager
    {
    public:
        using shared_connection = std::shared_ptr <tcp_connection_interface>;

        ~connection_manager();

        void add(shared_connection connection);
        void remove(shared_connection connection);

    private:
        std::unordered_set <shared_connection> connections_;
    };
}
