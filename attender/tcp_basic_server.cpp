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
    void tcp_basic_server::get(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("GET", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::put(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("PUT", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::post(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("POST", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::head(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("HEAD", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::delete_(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("DELETE", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::options(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("OPTIONS", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::connect(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("CONNECT", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::route(std::string const& route_name, std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route(route_name, path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_basic_server::mount(
        std::string const& root_path,
        std::string const& path_template,
        mount_callback const& on_connect,
        mount_option_set const& supported_methods
    )
    {
        router_.mount(root_path, path_template, on_connect, supported_methods);
    }
//#####################################################################################################################
}
