#include <attender/websocket/server/server.hpp>

#include <optional>

namespace attender::websocket
{
//#####################################################################################################################
    struct server::implementation : public std::enable_shared_from_this<server::implementation>
    {
        boost::asio::io_context* service;
        boost::asio::ip::tcp::acceptor acceptor;
        boost::asio::ip::tcp::endpoint local_endpoint;
        std::function <void(boost::system::error_code)> on_error;
        std::function<void(std::shared_ptr<connection>)> on_connection;
        std::optional<boost::asio::ssl::context> security_context;

        implementation(
            boost::asio::io_context* service,
            std::function <void(boost::system::error_code)> on_error,
            std::optional <security_parameters> security_params
        )
            : service{service}
            , acceptor{*service}
            , local_endpoint{}
            , on_error{std::move(on_error)}
            , on_connection{}
            , security_context{[&security_params]() -> std::optional<boost::asio::ssl::context> {
                if (!security_params)
                    return std::nullopt;
                boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12};
                ctx.set_password_callback([password = security_params->passphrase]
                    (std::size_t, boost::asio::ssl::context_base::password_purpose) {
                        return password;
                    }
                );
                ctx.set_options(
                    boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                    boost::asio::ssl::context::single_dh_use
                );
                ctx.use_certificate_chain(
                    boost::asio::buffer(security_params->cert.data(), security_params->cert.size())
                );
                ctx.use_private_key(
                    boost::asio::buffer(security_params->key.data(), security_params->key.size()),
                    boost::asio::ssl::context::file_format::pem
                );
                if (!security_params->diffie_hellman_parameters.empty()) {
                    ctx.use_tmp_dh(boost::asio::buffer(
                        security_params->diffie_hellman_parameters.data(), security_params->diffie_hellman_parameters.size()));
                }
                return ctx;
            }()}
        {
        }

        void do_accept()
        {
            acceptor.async_accept(
                boost::asio::make_strand(*service),
                [weak = weak_from_this()](boost::system::error_code ec, boost::asio::ip::tcp::socket&& socket)
                {
                    if (auto shared = weak.lock(); shared)
                    {
                        if (ec)
                        {
                            shared->on_error(ec);
                        }
                        if (shared->security_context)
                        {
                            std::make_shared<connection>(
                                shared->service, 
                                std::move(socket), 
                                shared->on_error, 
                                shared->on_connection,
                                *shared->security_context
                            )->start();
                        }
                        else
                        {
                            std::make_shared<connection>(
                                shared->service, 
                                std::move(socket), 
                                shared->on_error, 
                                shared->on_connection
                            )->start();
                        }
                        shared->do_accept();
                    }
                }
            );
        }
    };
//#####################################################################################################################
    server::server(boost::asio::io_context* service, std::function <void(boost::system::error_code)> on_error)
        : impl_{std::make_shared <implementation>(service, std::move(on_error), std::nullopt)}
    {        
    }
//---------------------------------------------------------------------------------------------------------------------
    server::server(
        boost::asio::io_context* service, 
        std::function <void(boost::system::error_code)> on_error, 
        security_parameters const& params
    )
        : impl_{std::make_shared <implementation>(service, std::move(on_error), params)}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    server::~server()
    {
        stop();
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::endpoint server::local_endpoint() const
    {
        return impl_->acceptor.local_endpoint();
    }
//---------------------------------------------------------------------------------------------------------------------
    void server::start(
        std::function<void(std::shared_ptr<connection>)> on_connection,
        std::string const& port, 
        std::string const& host
    )
    {
        stop();

        boost::asio::ip::tcp::resolver resolver{*impl_->service};

        impl_->on_connection = on_connection;

        impl_->local_endpoint = *resolver.resolve(host, port);
        impl_->acceptor.open(impl_->local_endpoint.protocol());
        impl_->acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        impl_->acceptor.bind(impl_->local_endpoint);
        impl_->acceptor.listen();

        impl_->do_accept();
    }
//---------------------------------------------------------------------------------------------------------------------
    void server::stop()
    {
        impl_->acceptor.close();
    }
//#####################################################################################################################
}