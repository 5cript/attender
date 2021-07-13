#include <attender/http/http_basic_server.hpp>
#include <attender/http/response.hpp>

#include <iostream>

namespace attender
{
//#####################################################################################################################
    http_basic_server::http_basic_server(asio::io_service* service,
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
    http_basic_server::~http_basic_server()
    {
        stop();
    }
//---------------------------------------------------------------------------------------------------------------------
    connection_manager* http_basic_server::get_connections()
    {
        return &connections_;
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::endpoint http_basic_server::get_local_endpoint() const
    {
        return acceptor_.local_endpoint();
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::start(std::string const& port, std::string const& host)
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
    void http_basic_server::stop()
    {
        acceptor_.close();
    }
//---------------------------------------------------------------------------------------------------------------------
    settings http_basic_server::get_settings() const
    {
        return settings_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::install_session_control
    (
        SessionControlParam&& controlParam
    )
    {
        sessionControl_.id_cookie_key = controlParam.id_cookie_key;
        sessionControl_.sessions = std::make_shared <session_manager>(std::move(controlParam.session_storage));
        sessionControl_.authorizer = std::shared_ptr <authorizer_interface>(controlParam.authorizer.release());
        sessionControl_.authorization_conditioner = controlParam.authorization_conditioner;
        router_.add_session_manager(sessionControl_.sessions, sessionControl_.id_cookie_key);
        sessionControl_.allowOptionsUnauthorized = controlParam.allowOptionsUnauthorized;
        sessionControl_.cookie_base = controlParam.cookie_base;
        sessionControl_.disable_automatic_authentication = controlParam.disable_automatic_authentication;
        sessionControl_.authentication_path = controlParam.authentication_path;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::weak_ptr <session_manager> http_basic_server::get_installed_session_manager()
    {
        if (!sessionControl_.sessions)
            return {};
        return sessionControl_.sessions;
    }
//---------------------------------------------------------------------------------------------------------------------
    session_manager* http_basic_server::get_session_manager()
    {
        return sessionControl_.sessions.get();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool http_basic_server::authenticate_session(request_handler* req, response_handler* res)
    {
        if (sessionControl_.authorization_conditioner)
            sessionControl_.authorization_conditioner(req, res);

        auto observer = res->observe_conclusion();
        auto result = sessionControl_.authorizer->try_perform_authorization(req, res);
        if (observer->has_concluded())
            return false;

        auto make_session = [this, res, req]()
        {
            auto id = sessionControl_.sessions->make_session();
            cookie c = sessionControl_.cookie_base;
            c.set_name(sessionControl_.id_cookie_key);
            c.set_value(id);
            c.set_path("/");
            res->set_cookie(c);
            req->patch_cookie(sessionControl_.id_cookie_key, id);
            return true;
        };

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
                sessionControl_.authorizer->negotiate_authorization_method(req, res);
                return false;
            }
            case(authorization_result::allowed_continue):
            {
                make_session();
                return true;
            }
            case(authorization_result::allowed_but_stop):
            {
                make_session();
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
            {
                res->end();
                return false;
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    bool http_basic_server::handle_session(request_handler* req, response_handler* res)
    {
        if (!sessionControl_.sessions)
            return true;

        auto state = sessionControl_.sessions->load_session <attender::session>(sessionControl_.id_cookie_key, nullptr, req);
        if (state == session_state::live)
            return true;

        if (sessionControl_.allowOptionsUnauthorized && req->method() == "OPTIONS")
            return true;

        if (sessionControl_.disable_automatic_authentication)
        {
            if (req->path() != sessionControl_.authentication_path)
            {
                res->status(401).end();
                return false;
            }
            return true;
        }

        return authenticate_session(req, res);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::get(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("GET", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::put(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("PUT", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::post(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("POST", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::head(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("HEAD", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::delete_(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("DELETE", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::options(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("OPTIONS", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::connect(std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route("CONNECT", path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::route(std::string const& route_name, std::string const& path_template, connected_callback const& on_connect)
    {
        router_.add_route(route_name, path_template, on_connect);
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::mount(
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
    void http_basic_server::header_read_handler(request_handler* req, response_handler* res, http_connection_interface* connection, boost::system::error_code ec, std::exception const& exc)
    {
        auto clearConnection = [this, connection]()
        {
            connections_.remove(connection);
        };

        // socket closed
        if (ec.value() == 2)
            return clearConnection();

        if (ec == boost::asio::error::operation_aborted)
            return clearConnection();

        if (ec)
        {
            on_error_(connection, ec, exc);
            if (ec.value() == boost::system::errc::protocol_error)
                return (void)connection->get_response_handler().send_status(400);
            else
                return clearConnection();
        }

        // finished header parsing.
        match_result best_match;
        auto maybeRoute = router_.find_route(req->get_header(), best_match);
        if (maybeRoute)
        {
            req->set_parameters(maybeRoute.get().get_path_parameters(req->get_header().get_path()));
            if (handle_session(req, res))
            {
                try
                {
                    maybeRoute.get().get_callback()(req, res);
                }
                catch(std::exception const& exc)
                {
                    if (settings_.expose_exception)
                        res->status(500).send(exc.what());
                    else
                        res->status(500).end();
                }
                catch(...)
                {
                    res->send_status(500);
                }
            }
        }
        else
        {
            if (best_match == match_result::path_match)
                res->send_status(405);
            else if (on_missing_handler_)
                on_missing_handler_(req, res);
            else
                res->send_status(404);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void http_basic_server::mount(
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
