#include <attender/http/http_secure_connection.hpp>

namespace attender
{
//#####################################################################################################################
    http_secure_connection::http_secure_connection
    (
        http_server_interface* parent, 
        http_secure_connection::ssl_socket_type* socket,
        final_callback const& on_timeout
    )
        : http_connection_base <http_secure_connection::ssl_socket_type> (parent, socket, on_timeout)
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void http_secure_connection::shutdown()
    {
        boost::system::error_code ignored_ec;
        socket_->shutdown(ignored_ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    http_secure_connection::ssl_socket_type* http_secure_connection::get_secure_socket()
    {
        return &*socket_;
    }
//#####################################################################################################################
}
