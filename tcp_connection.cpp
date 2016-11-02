#include "tcp_connection.hpp"
#include "debug.hpp"

#include <algorithm>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_connection::tcp_connection(tcp_server_interface* parent, boost::asio::ip::tcp::socket&& socket)
        : tcp_connection_base <boost::asio::ip::tcp::socket> (parent, std::move(socket), std::move(socket))
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_connection::shutdown()
    {
        boost::system::error_code ignored_ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }
//#####################################################################################################################
}
