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
    /**
     *  The server base, that combines common functionality of all server types.
     */
    class tcp_basic_server : public tcp_server_interface
    {
    public:
        /**
         *  Creates a tcp_basic_server.
         *
         *  @param the boost::asio::io_service. Every server needs to know it's io_service (see boost asio documentation)
         *  @param error_callback The error callback is called whenever an exception is thrown somewhere inside the server or connection internals.
         *                        The affected connection will be closed, or the server stopped, if thrown outside of a connection context.
         *  @param settings The settings of the server. @see attender::settings
         */
        tcp_basic_server(asio::io_service* service,
                         error_callback on_error,
                         settings setting = {});

        /**
         *  Destructor
         */
        ~tcp_basic_server();

        /**
         *  Starts the server on the port (0-65536 or "http", abides boost asio behaviour) on the interface "host".
         *
         *  @param port The port to bind to.
         *  @param host The target interface. "0.0.0.0" is the default value meaning, that it shall bind on any interface.
         */
        void start(std::string const& port, std::string const& host = "0.0.0.0") override;

        /**
         *  Stops the server and terminates all connections.
         *  What happens to connections amidst transactions is undefined. Operations will be aborted and streams closed.
         */
        void stop() override;

        /**
         *  Returns a copy of the server settings.
         */
        settings get_settings() const override;

        /**
         *  Will add a routing for get requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void get(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for put requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void put(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for post requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void post(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for head requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void head(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for delete_ requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void delete_(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for options requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void options(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for connect requests.
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void connect(std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Will add a routing for a custom requests method string (they cannot contain spaces).
         *
         *  @param path_template A template for paths. These templates will be parsed and if a match occurs in a request, the routing will be used.
         *  @param connect_callback A callback which gets called upon a request is received, that matches the path_template.
         */
        void route(std::string const& route_name, std::string const& path_template, connected_callback const& on_connect);

        /**
         *  Returns a pointer to the connection manager. It holds all active connections.
         *
         *  @return Returns a pointer to the connection_manager.
         */
        connection_manager* get_connections() override;

        /**
         *  mounts a path on the local system, so it can be accessed by http requests.
         *  Any requests on that path will result in corresponding actions that can be enabled or disabled.
         *
         *  BY DEFAULT, ONLY GET, HEAD AND OPTIONS ARE ENABLED.
         *  A get request will load and transfer the file if it exists, 404 is returned otherwise.
         *  A put/post will create a file and fill it with the sent data.
         *  A delete request will delete the file specified
         *  A head request will only read file properties, but not transfer the file.
         *  An options request will reply with "get, put, post, delete, head"
         *  A connect request, or any other custom request, will not be routed.
         *
         *  The on_connect handler will be called prior to the execution, so the option exists to deny or abort request.
         *  After the handler finished, the return value will be checked. A return value of false will mean, that the connection is
         *  to be aborted. Do not close the connection on your or do a send operation on it.
         *
         *  All requests will be jailed to the path, but include subdirectories, except if disabled.
         *
         *  @param root_path The path to jail to.
         *  @param path_template Presume root_path is /home/user/bla and path_template is /home, then requests to /home will
         *                       be redirected to /home/user/bla
         *  @param on_connect A handler called before executing operations. The handler may return false, if e.g. dubious / unauthorized.
         */
        void mount(std::string const& root_path,
                   std::string const& path_template,
                   mount_callback const& on_connect,
                   mount_option_set const& supported_methods = {mount_options::GET, mount_options::HEAD, mount_options::OPTIONS});

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
