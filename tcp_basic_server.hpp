#pragma once

#include "net_core.hpp"
#include "tcp_fwd.hpp"
#include "tcp_server_interface.hpp"
#include "connection_manager.hpp"
#include "router.hpp"
#include "settings.hpp"
#include "session/session_cookie_generator_interface.hpp"

namespace attender
{
    class tcp_basic_server : public tcp_server_interface
    {
    public:
        tcp_basic_server(asio::io_service* service,
                         error_callback on_error,
                         settings setting = {});
        ~tcp_basic_server();
        void start(std::string const& port, std::string const& host = "0.0.0.0") override;
        void stop() override;
        settings get_settings() const override;

        void get(std::string path_template, connected_callback const& on_connect);
        connection_manager* get_connections() override;

    protected:
        virtual void do_accept() = 0;

    protected:
        // asio stuff
        asio::io_service* service_;
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::ip::tcp::endpoint local_endpoint_;

        connection_manager connections_;
        request_router router_;

        // other
        settings settings_;

        // callbacks
        error_callback on_error_;
        missing_handler_callback on_missing_handler_;
    };
}
