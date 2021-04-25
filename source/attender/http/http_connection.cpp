#include <attender/http/http_connection.hpp>

#include <algorithm>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    http_connection::http_connection
    (
        http_server_interface* parent, 
        boost::asio::ip::tcp::socket&& socket,
        final_callback const& on_timeout
    )
        : http_connection_base <boost::asio::ip::tcp::socket> (parent, std::move(socket), std::move(socket), on_timeout)
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void http_connection::shutdown()
    {
        boost::system::error_code ignored_ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }
//#####################################################################################################################
}
