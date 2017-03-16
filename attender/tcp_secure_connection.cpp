#include "tcp_secure_connection.hpp"

namespace attender
{
//#####################################################################################################################
    tcp_secure_connection::tcp_secure_connection(tcp_server_interface* parent, tcp_secure_connection::ssl_socket_type* socket)
        : tcp_connection_base <tcp_secure_connection::ssl_socket_type> (parent, socket)
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_secure_connection::shutdown()
    {
        boost::system::error_code ignored_ec;
        socket_->shutdown(ignored_ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_secure_connection::ssl_socket_type* tcp_secure_connection::get_secure_socket()
    {
        return &*socket_;
    }
//#####################################################################################################################
}
