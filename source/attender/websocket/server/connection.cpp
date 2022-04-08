#include <attender/websocket/server/connection.hpp>

#include <optional>

namespace attender::websocket
{
//#####################################################################################################################
    void proto_connection::on_upgrade()
    {
        // Turn http to ws by upgrade.
        with_stream_do([this](auto&& sock) {
            std::make_shared<connection>(
                ctx_, 
                std::move(sock), 
                std::move(on_accept_error_), 
                std::move(on_accept_),
                std::move(upgrade_request_)
            )->start();
        });
    }
//#####################################################################################################################
}