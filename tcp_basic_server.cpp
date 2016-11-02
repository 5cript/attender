#include "tcp_basic_server.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_basic_server::tcp_basic_server(asio::io_service* service,
                           error_callback on_error,
                           settings setting)
        : service_{service}
        , acceptor_{*service}
        , local_endpoint_{}
        , connections_{}
        , router_{}
        , settings_{std::move(setting)}
        , on_error_{std::move(on_error)}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    tcp_basic_server::~tcp_basic_server()
    {
        stop();
    }
//---------------------------------------------------------------------------------------------------------------------
    connection_manager* tcp_basic_server::get_connections()
    {
        return &connections_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::start(std::string const& port, std::string const& host)
    {
        stop();

        boost::asio::ip::tcp::resolver resolver{*service_};
        local_endpoint_ = *resolver.resolve({host, port});
        acceptor_.open(local_endpoint_.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(local_endpoint_);
        acceptor_.listen();

        do_accept();
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::stop()
    {
        acceptor_.close();
    }
//---------------------------------------------------------------------------------------------------------------------
    settings tcp_basic_server::get_settings() const
    {
        return settings_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::get(std::string path_template, connected_callback const& on_connect)
    {
        router_.add_route("GET", path_template, on_connect);
    }
//#####################################################################################################################
}
