#include "tcp_basic_server.hpp"
#include "response.hpp"

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

        local_endpoint_ = *resolver.resolve(host, port);
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
    void tcp_basic_server::install_session_control
    (
        std::unique_ptr <session_storage_interface>&& session_storage,
        std::unique_ptr <authorizer_interface>&& authorizer,
        std::string const& id_cookie_key
    )
    {
        id_cookie_key_ = id_cookie_key;
        sessions_ = std::make_shared <session_manager>(std::move(session_storage));
        authorizer_ = std::shared_ptr <authorizer_interface>(authorizer.release());
        router_.add_session_manager(sessions_, id_cookie_key_);
    }
//---------------------------------------------------------------------------------------------------------------------
    session_manager* tcp_basic_server::get_session_manager()
    {
        return sessions_.get();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool tcp_basic_server::handle_session(request_handler* req, response_handler* res)
    {
        if (!sessions_)
            return true;

        auto state = sessions_->load_session <attender::session>(id_cookie_key_, nullptr, req);
        if (state == session_state::live)
            return true;

        auto observer = res->observe_conclusion();
        auto result = authorizer_->try_perform_authorization(req, res);
        if (observer->has_concluded())
            return false;

        switch (result)
        {
            case(authorization_result::denied):
            {
                if (!observer->has_concluded())
                    res->status(401).end();
                return false;
            }
            case(authorization_result::negotiate):
            {
                authorizer_->negotiate_authorization_method(req, res);
                return false;
            }
            case(authorization_result::allowed_continue):
            {
                sessions_->make_session();
                return true;
            }
            case(authorization_result::allowed_but_stop):
            {
                if (!observer->has_concluded())
                    res->status(204).end();
                return false;
            }
            case(authorization_result::bad_request):
            {
                if (!observer->has_concluded())
                    res->status(400).end();
                return false;
            }
            default:
                return false;
        }
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
       mount_callback_2 const& on_connect,
       mount_option_set const& supported_methods,
       int priority
    )
    {
        router_.mount(root_path, path_template, on_connect, supported_methods, priority);
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
