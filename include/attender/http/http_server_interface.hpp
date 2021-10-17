#pragma once

#include <attender/http/settings.hpp>
#include <attender/http/http_fwd.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <string>

namespace attender
{
    class http_server_interface
    {
    public:
        virtual ~http_server_interface() = default;

        virtual void start(std::string const& port, std::string const& host) = 0;
        virtual void stop() = 0;
        virtual boost::asio::ip::tcp::endpoint get_local_endpoint() const = 0;
        virtual settings get_settings() const = 0;
        virtual connection_manager* get_connections() = 0;
    };
}
