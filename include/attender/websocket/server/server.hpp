#pragma once

#include <attender/net_core.hpp>
#include <attender/websocket/server/connection.hpp>

#include <string>
#include <memory>
#include <functional>

namespace attender::websocket
{

class server
{
public:
    server(boost::asio::io_context* service, std::function <void(boost::system::error_code)> on_error);

    ~server();

    /**
     *  Starts the server on the given port on the interface "host".
     *
     *  @param port The port to bind to.
     *  @param host The target interface. "0.0.0.0" is the default value for ipv4 and "::" for ipv6.
     */
    void start(
        std::function<void(std::shared_ptr<connection>)> on_connection,
        std::string const& port = "0", 
        std::string const& host = "::"
    );
    
    /**
     *  Stops the server and terminates all connections.
     */
    void stop();

private:
    struct implementation;
    std::shared_ptr <implementation> impl_;
};

}